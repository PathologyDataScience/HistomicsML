import pandas as pd
import sys,os
import csv
import json
import numpy as np
from math import *
import multiprocessing
from lifelines import CoxPHFitter
from lifelines.utils import k_fold_cross_validation

def check_if_binary(arr):
	uni = np.unique(arr)
	return (uni.shape[0] == 2)

def standardization(arr):
	return (arr-arr.mean())/arr.std()

def round_dic(ori_dic):
	for key in ori_dic:
		ori_dic[key] = round(ori_dic[key], 2)
	return ori_dic
def round_dic_eng(ori_dic):
	for key in ori_dic:
		ori_dic[key] = powerise10(ori_dic[key])
	return ori_dic
def powerise10(x):
    """ Returns x as a*10**b with 0 <= a < 10
    """
    if x == 0: return 0,0
    Neg = x < 0
    if Neg: x = -x
    a = 1.0 * x / 10**(floor(log10(x)))
    b = int(floor(log10(x)))
    if Neg: a = -a
    return round(a, 2) * (10 ** b)

def cox_regression(clean_df):
	cf = CoxPHFitter()
	cf.fit(clean_df, 'time', event_col='event')
	summary_df = cf.summary
	#decimals = pd.Series([2, 2, 2], index=['exp(coef)', 'lower 0.95', 'upper 0.95'])
	#summary_df = summary_df.round(decimals)
	ori_dic = summary_df.to_dict()
	res_dic= {}
	for stat_of_interest in stats_of_interest:
		if stat_of_interest != 'p':
			res_dic[stat_of_interest] = round_dic(ori_dic[stat_of_interest])
		else:
			res_dic[stat_of_interest] = round_dic_eng(ori_dic[stat_of_interest])
	return res_dic

def generate_clean_df(raw_df):
	keys = raw_df['key']
	values = raw_df['value']
	num_keys = len(keys)
	data_records = [(keys[i], np.array(values[i], dtype=float)) for i in range(num_keys)]
	processed_df = pd.DataFrame.from_items(data_records)
	#drop patients with NaN features
	clean_df = processed_df.dropna(axis=0, how='any')
	#transform the original data to meet the default of linelines
	clean_df.loc[:, 'event'] = -1 * clean_df.loc[:, 'event']+ 1
	num_coef = clean_df.shape[1] - 2 #exclude time and event columns
	assert num_coef > 0

	for i in range(2, num_coef + 2):
		#standardization if necessary
		if not (check_if_binary(clean_df.iloc[:,i])):
			clean_df.iloc[:,i] = standardization(clean_df.iloc[:,i])
	return clean_df

def generate_csv(result_dic):
		val = result_dic.values()
		val_num = len(val)
		var_names = []
		var_names.append("Variable")
		for key in result_dic:
			if(key =="exp(coef)"):
				var_names.append("Hazard Ratio")
			else :
				var_names.append(key)
		col_name = []
		total_ll = []
		ll = result_dic.values()[0]
		for i in range(len(ll.keys())):
			col_name.append(ll.keys()[i])
		total_ll.append(var_names)
		# import pdb; pdb.set_trace()
		for item in col_name:
			row = []
			row.append(item)
			for i in range(val_num):
				ls = val[i][row[0]]
				row.append(ls)
			total_ll.append(row)
		return total_ll
		# import pdb; pdb.set_trace()

def write_csv(file_name,total_ll):
	with open(file_name, "w+") as f:# opening the file with w+ mode truncates the file
		writer = csv.writer(f)
		writer.writerows(total_ll)
	f.close()

def rearrange_col_names(df):
	columns = df.columns
	ind_time = df.columns.get_loc('time')
	ind_event = df.columns.get_loc('event')
	colnames = df.columns.tolist()
	if not (ind_time == 0):
		tmp = colnames[0]
		colnames[0] = colnames[ind_time]
		colnames[ind_time] = tmp
	if not (ind_event == 1):
		tmp = colnames[0]
		colnames[0] = colnames[ind_event]
		colnames[ind_event] = tmp
	return df.ix[:, colnames]


if __name__ == '__main__':
	#global variable
	stats_of_interest = ['exp(coef)', 'p','lower 0.95', 'upper 0.95']
	data = sys.argv[1]

	#uni-variate
	if (sys.argv[2] == 'uni'):
		# data = json.loads(sys.argv[1])
		df = pd.read_json(data)
		df.columns = df.columns.str.replace('\r','')
		# import pdb; pdb.set_trace()
		# colms = df.columns
		# colms = [i.strip() for i in colms]
		# df = df.ix[:, colms]
		df = df.dropna(axis=0,how='any')
		df.loc[:, 'event'] = -1 * df.loc[:, 'event']+ 1
		clean_df = rearrange_col_names(df)
		result_dic = {}
		for stat_of_interest in stats_of_interest:
			result_dic[stat_of_interest] = {}
		truncated_dataFrames = [clean_df.iloc[:,[0,1,i]] for i in range(2,clean_df.shape[1])]
		pool = multiprocessing.Pool()
		for sub_res_dict in pool.imap_unordered(cox_regression, truncated_dataFrames):
			for stat_of_interest in stats_of_interest:
				result_dic[stat_of_interest].update(sub_res_dict[stat_of_interest])
		pool.close()
		uni_dt = pd.DataFrame.from_dict(result_dic)
		uni_dt.columns.values[0] = 'Hazard Ratio'
		col_list = ['Hazard Ratio', 'lower 0.95', 'upper 0.95', 'p']
		uni_dt = uni_dt.ix[:, col_list]
		uni_dt.to_csv("uni_output.csv",mode='w+',index_label="Variable")
	#multi_variate
	elif (sys.argv[2] == 'multi'):
		df = pd.read_json(data)
		df.columns = df.columns.str.replace('\r','')
		clean_df = generate_clean_df(df)
		uni_df = pd.DataFrame.from_csv("uni_output.csv")
		# import pdb; pdb.set_trace()
		multi_result_dic = cox_regression(clean_df)
		multi_df = pd.DataFrame.from_dict(multi_result_dic)
		multi_df.columns.values[0] = 'Hazard Ratio'
		col_list = ['Hazard Ratio', 'lower 0.95', 'upper 0.95', 'p']
		multi_df = multi_df.ix[:, col_list]
		result = pd.concat([uni_df,multi_df], axis=1)
		result.to_csv("multi_output.csv",mode='w+',index_label="Variable")

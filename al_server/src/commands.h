//
//	Copyright (c) 2014-2017, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//
//
#if !defined(SRC_COMMANDS_H_)
#define SRC_COMMANDS_H_


// JSON object tags
//
#define UID_TAG		"uid"
#define CMD_TAG		"command"


// Server commands
//
#define CMD_INIT		"init"
#define CMD_END			"end"
#define CMD_FINAL		"finalize"
#define CMD_CLASSINIT	"viewerLoad"
#define CMD_CLASSEND	"viewerEnd"
#define CMD_PRIME		"prime"
#define CMD_SELECT		"select"
#define CMD_SUBMIT		"submit"
#define CMD_APPLY		"apply"
#define CMD_VISUAL		"visualize"
#define CMD_RELOAD		"reload"

#define CMD_PICKINIT	"pickerInit"
#define CMD_PICKADD		"pickerAdd"
#define CMD_PICKCNT		"pickerCnt"
#define CMD_PICKEND		"pickerSave"
#define CMD_PICKREVIEW		"pickerReview"
#define CMD_PICKREVIEWSAVE		"pickerReviewSave"
#define CMD_PICKRELOAD	"pickerReload"
#define CMD_VIEWLOAD	"viewerLoad"

#define CMD_HEATMAP		"heatMap"
#define CMD_ALLHEATMAPS	"allHeatMaps"
#define CMD_SREGIONHEATMAP		"sregionHeatMap"
#define CMD_SREGIONALLHEATMAPS	"sregionallHeatMaps"

#define CMD_STATUS		"sysStatus"

#define CMD_REVIEW		"review"
#define CMD_REVIEWSAVE		"reviewSave"

#endif /* SRC_COMMANDS_H_ */

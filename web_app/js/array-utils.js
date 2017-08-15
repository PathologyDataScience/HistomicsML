/**
 * E.g. if operator = "gte" (greater-than-or-equal), returns an array of Booleans, where the i-th element is True if and only if x[i] >= val.
 * The operator can be: "eq", "lt", "gt", "lte" or "gte".
 */
function compare(x, operator, val){
    var res = [];
    for (var i = 0; i < x.length; i++){
        if (operator == "eq" ) res.push(x[i] == val);
        if (operator == "lt" ) res.push(x[i] <  val);
        if (operator == "gt" ) res.push(x[i] >  val);
        if (operator == "lte") res.push(x[i] <= val);
        if (operator == "gte") res.push(x[i] >= val);
    }
    return res;
}

/**
 * Returns a subset of the array x according to select, an array of Booleans of length equal to the length of x.
 */
function subset(x, select){
    var sub = [];
    for (var i = 0; i < x.length; i++){
        if (select[i]) sub.push(x[i]);
    }
    return sub;
}

/**
 * Appends array b to the end of a copy of array a.
 */
function joinArrays(a, b){
    var acopy = a.slice();
    Array.prototype.push.apply(acopy, b);
    return acopy
}

/**
 * Converts an array of x-axis values and an array of y-axis values into an array of coordinates.
 * E.g. xval=[1, 2, 3] and yval=[11, 12, 13] -> [[xscale(1), yscale(11)], [xscale(2), yscale(12)], [xscale(3), yscale(13)]]
 */
function arrayOf2DimArrays(xval, yval, xscale, yscale){
    var arr = [];
    for (var i = 0; i < xval.length; i++){
        arr.push([xscale(xval[i]), yscale(yval[i])]);
    }
    return arr;
}

/**
 * Removes duplicates from an array. E.g. [1, 3, 1] --> [1, 3].
 * Thanks to http://jszen.com/best-way-to-get-unique-values-of-an-array-in-javascript.7.html.
 */
function unique(x){
    var n = {}, r = [];
    for (var i = 0; i < x.length; i++){
        if (!n[x[i]]){
            n[x[i]] = true; 
            r.push(x[i]); 
        }
    }
    return r;
}

/**
 * Returns an array of lenth n, with x in each spot.
 */
function rep(x, n){
    return Array.apply(null, new Array(n)).map(function(){return x})
}

/**
 * Returns true if and only if x is in the array arr. Otherwise returns false.
 */
function inArray(x, arr){
    return ($.inArray(x, arr) >= 0)
}
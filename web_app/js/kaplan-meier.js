/**
 * Calculates confidence intervals for the supplied Kaplan-Meier estimates.
 *
 * @param  array  kaplanMeier An array of Kaplan-Meier estimates. 
 * @param  array  stdError    The standard errors of the Kaplan-Meier estimates. Must be the same length as kaplanMeier.
 * @param  float  z           The z-score, e.g. 1.96 gives a 95% confidence interval.
 * @param  string ciType      The type of confidence interval: "ordinary", "log" or "loglog".
 * @return array  An 2-dimensional array of (a) lower confidence interval values and (b) upper confidence interval values.
 *                Each of these arrays is the same length as the supplied array of Kaplan-Meier estimates.
 */
function ciKaplanMeier(kaplanMeier, stdError, z, ciType){
    var n = kaplanMeier.length;
    var lowerCI = new Array(n);
    var upperCI = new Array(n);
        
    for (var i = 0; i < n; i++){
        var mean = kaplanMeier[i];
        if (ciType == "log")    mean = Math.log(mean);
        if (ciType == "loglog") mean = Math.log(-Math.log(mean));
        lowerCI[i] = mean - z * stdError[i];
        upperCI[i] = mean + z * stdError[i];
        if (ciType == "log"){
            lowerCI[i] = Math.exp(lowerCI[i]);
            upperCI[i] = Math.exp(upperCI[i]);
        }
        if (ciType == "loglog"){
            lowerCI[i] = Math.exp(-Math.exp(lowerCI[i]));
            upperCI[i] = Math.exp(-Math.exp(upperCI[i]));
        }
        lowerCI[i] = Math.max(0, lowerCI[i]);
        upperCI[i] = Math.min(1, upperCI[i]);
    }
    return [lowerCI, upperCI];
}

/**
 * Calculates the Kaplan-Meier estimator of the survival function for right-censored data. 
 *
 * Also calculates the Greenwood estimator of the standard error of 
 * (a) the Kaplan-Meier estimator
 * (b) the log Kaplan-Meier estimator,     i.e. the standard error of ln(KM(t)),      where KM is the Kaplan-Meier estimator.
 * (c) the log-log Kaplan-Meier estimator, i.e. the standard error of ln(-ln(KM(t))), where KM is the Kaplan-Meier estimator.
 *
 * Also returns the number of subjects at risk at each time point, the number that experienced an event and the number that censored.
 *
 * @param  array times  An array of non-negative floats or ints giving the times at which events or censoring occurred. 
 * @param  array events An array of Booleans. False for censoring, true for an event.
 * @return object       An object containing these equal-length arrays:
 *                      * time:              Non-negative times at which events or censoring occurred. Sorted in ascending order.
 *                      * nAtRisk:           The number of subjects at risk at each time point.
 *                      * nEvents:           The number of subjects experiencing an event at each time point.
 *                      * nCensored:         The number of subjects censoring at each time point.
 *                      * kaplanMeier:       Kaplan-Meier estimates at each time point.
 *                      * greenwoodSE:       Greenwood estimates of the standard error of those Kaplan-Meier estimates.
 *                      * greenwoodSELog:    Greenwood estimates of the standard error of the log Kaplan-Meier estimates. 
 *                      * greenwoodSELogLog: Greenwood estimates of the standard error of the log-log Kaplan-Meier estimates.
 */
function kaplanMeier(times, events){
    function numAsc(first, second){return first - second;} // Use this for sorting.

    // Returned variables:
    var uniqueTimes = unique(times).sort(numAsc); // Event or censor times, sorted in ascending order.
    var kms       = new Array(uniqueTimes.length); // kms[i]       is the Kaplan-Meier estimate at uniqueTimes[i].
    var gws       = new Array(uniqueTimes.length); // gws[i]       is the Greenwood estimate of the standard error of the Kaplan-Meier estimate at uniqueTimes[i].
    var gwls      = new Array(uniqueTimes.length); // gwls[i]      is the Greenwood estimate of the standard error of the log Kaplan-Meier estimate at uniqueTimes[i].    
    var gwlls     = new Array(uniqueTimes.length); // gwlls[i]     is the Greenwood estimate of the standard error of the log-log Kaplan-Meier estimate at uniqueTimes[i].
    var nAtRisk   = new Array(uniqueTimes.length); // nAtRisk[i]   is the number of subjects at risk at uniqueTimes[i].
    var nEvents   = new Array(uniqueTimes.length); // nEvents[i]   is the number of subjects experiencing an event at uniqueTimes[i].
    var nCensored = new Array(uniqueTimes.length); // nCensored[i] is the number of subjects who censor at at uniqueTimes[i].    

    var gwSum = 0; // A cumulative sum used to calculate the Greenwood estimate of the standard error.
 
    // Sort times in ascending order. And reorder events accordingly.
    events = _sortFirstBySecond(events, times);
    times.sort(numAsc);
    
    // Loop across the time points at which an event occurred or at which a subject was censored. 
    for (var iTime = 0; iTime < uniqueTimes.length; iTime++){
        
        // Get the number of subjects at risk at uniqueTimes[iTime].  
        nAtRisk[iTime] = times.length;  
        
        // Get the number of subjects experiencing an event at uniqueTimes[iTime]. 
        // And find the index of the first element in the (sorted) times array greater than uniqueTimes[iTime].
        nEvents[iTime]   = 0;
        nCensored[iTime] = 0;
        for (var i = 0; i < times.length; i++){
            if (times[i] == uniqueTimes[iTime]){
                nEvents[iTime]   +=  events[i];
                nCensored[iTime] += !events[i];
            }
            else break;
        }
        var idxNextTimePoint = i;
        
        // Get the Kaplan-Meier estimate at uniqueTimes[iTime].
        kms[iTime] = (1 - nEvents[iTime] / nAtRisk[iTime]) * ((iTime == 0) ? 1 : kms[iTime - 1]);
        
        // Get the Greenwood estimates of the standard error at uniqueTimes[iTime].
        gwSum += nEvents[iTime] / (nAtRisk[iTime] * (nAtRisk[iTime] - nEvents[iTime]));
        gws[iTime]   = Math.sqrt(gwSum) * kms[iTime];
        gwls[iTime]  = Math.sqrt(gwSum);
        gwlls[iTime] = Math.sqrt(gwSum) / Math.log(kms[iTime]);
        
        // Remove from times and events data for subjects who experienced an event or were censored at uniqueTimes[iTime].
        events = events.slice(idxNextTimePoint);
        times  =  times.slice(idxNextTimePoint);
    }
    
    return {"time"             : uniqueTimes,
            "nAtRisk"          : nAtRisk, 
            "nEvents"          : nEvents, 
            "nCensored"        : nCensored, 
            "kaplanMeier"      : kms, 
            "greenwoodSE"      : gws,
            "greenwoodSELog"   : gwls,            
            "greenwoodSELogLog": gwlls};
}

/**
 * Orders first array by the second (numeric) array.
 * E.g. _sortFirstBySecond(['a', 'b', 'c', 'd', 'e'], [8, 99, 6, 6, 5]) returns ['e', 'c', 'd', 'a', 'b'] or ['e', 'd', 'c', 'a', 'b'];
 */
function _sortFirstBySecond(first, second){
    firstSecond = _zip(first, second)
    firstSecond.sort(function(a, b){return a[1] - b[1]});
    return _unzip(firstSecond, 0);
}

/**
 * E.g. _zip([1, 2, 3], ['a', 'b', 'c']) returns [[1, 'a'], [2, 'b'], [3, 'c']]
 */
function _zip(a, b){
    return a.map(function(e, i){return [a[i], b[i]];});
}

/**
 * E.g. _unzip([[1,2],[3,4],[5,6]], 1) returns [2, 4, 6]
 */
function _unzip(x, idx){
    return x.map(function(e, i){return e[idx];});
}

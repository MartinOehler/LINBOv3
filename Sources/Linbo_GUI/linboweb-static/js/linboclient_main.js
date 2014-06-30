// Author: Martin Oehler <oehler@knopper.net> 2013
// License: GPL V2

var linbocmdisrunning=0;
var linbocmdintervalid=0;

var autostarttimeoutid=0;
var autostartintervalid=0;

function updateLog() {
    if( document.getElementById('logpanelcheckbox').checked == 1 ) {
	Dajaxice.linboweb.linboclient.displaylog(Dajax.process);
	// var log = $('#log');
	// log.scrollTop( log[0].scrollHeight - log.height() );
    }
}

function appendLog(line) {
    var log = $('#log');
    log.append(line);
    log.scrollTop( log[0].scrollHeight - log.height() );
}

setInterval(function(){
    updateLog();
},3000);

function isEmpty(str) {
    return (!str||!/\S/.test(str));
}

function documentisready() {
    // build our page, check for admin login
    Dajaxice.linboweb.linboclient.loadmaindiv(Dajax.process);

    // loadmaindiv will restart the autostart panel
    $("#autostartpanel").hide();
    
}

function linbocmd(command,osname) {
 
    startlinbocmdinterval();
    linbocmdsetrunning();
    
    // linbo_cmd now has 5 seconds to get up and running
    Dajaxice.linboweb.linboclient.shell(Dajax.process, {'command':command,'osname':osname});
    
    return false;
}

function linbocmdmkcloop(partition,
			 imagepath) {

    startlinbocmdinterval();
    linbocmdsetrunning();
    
    // linbo_cmd now has 5 seconds to get up and running
    Dajaxice.linboweb.linboclient.mkcloop(Dajax.process, {'partition':partition,
							  'imagepath':imagepath});
}

function linbocmdupload(imagepath) {

    startlinbocmdinterval();
    linbocmdsetrunning();
    
    // linbo_cmd now has 5 seconds to get up and running
    Dajaxice.linboweb.linboclient.upload(Dajax.process, {'imagepath':imagepath});
}

function startlinboautostartinterval(sec) {
    $("#autostartprogressbar").prop('value',sec);
    $("#autostartprogressbar").prop('max',sec);
   
    autostartintervalid = setInterval(function(){$("#autostartprogressbar").prop('value',$("#autostartprogressbar").prop('value')-1)},1000);
}

function startlinboautostarttimeout(command,osname,sec) {
    $("#autostarttext").text("Countdown for "+command+" "+osname);

    autostarttimeoutid = setTimeout(function(){linbocmd(command,osname); cancelautostart()},(sec*1000));

    $(".managebutton").prop('disabled',true);
    $(".createbutton").prop('disabled',true);
    $(".uploadbutton").prop('disabled',true);
    $(".startbutton").prop('disabled',true);
    $(".syncstartbutton").prop('disabled',true);
    $(".quicksyncbutton").prop('disabled',true);
    $(".cachesyncbutton").prop('disabled',true);
    $(".gpartedbutton").prop('disabled',true);

}

function startlinbocmdinterval() {
    // start process watcher
    linbocmdintervalid = setInterval(function(){Dajaxice.linboweb.linboclient.linbocmdrunning(Dajax.process)},5000);
}

function progresspanelsetstatusline(line) {
    $("#progressbarstatusline").text(line);
}

function login() {
    Dajaxice.linboweb.linboclient.linbologin(Dajax.process, {'username':$("#username").val(),'password':$("#password").val()});
    return false;
}


function linbocmdsetrunning() {
    linbocmdisrunning=1;
    $("#progresspanel").show();    
    // change button states
    $(".managebutton").prop('disabled',true);
    $(".createbutton").prop('disabled',true);
    $(".uploadbutton").prop('disabled',true);
    $(".startbutton").prop('disabled',true);
    $(".syncstartbutton").prop('disabled',true);
    $(".quicksyncbutton").prop('disabled',true);
    $(".cachesyncbutton").prop('disabled',true);
    $(".gpartedbutton").prop('disabled',true);
}


function linbocmdsetnotrunning() {
    linbocmdisrunning=0;
    $("#progresspanel").hide();

    clearInterval(linbocmdintervalid);
    // change button states
    $(".managebutton").prop('disabled',false);
    $(".createbutton").prop('disabled',false);
    $(".uploadbutton").prop('disabled',false);
    $(".startbutton").prop('disabled',false);
    $(".syncstartbutton").prop('disabled',false);
    $(".quicksyncbutton").prop('disabled',false);
    $(".cachesyncbutton").prop('disabled',false);
    $(".gpartedbutton").prop('disabled',false);
}


function cancelprocess() {
    Dajaxice.linboweb.linboclient.cancellinbocmd();

}

function schedulelinbocmd(command,os,timeout) {
    startlinboautostartinterval(timeout);
    startlinboautostarttimeout(command,os,timeout);
}

function cancelautostart() {
    $("#autostartpanel").hide();
    clearInterval(autostarttimeoutid);
    clearInterval(autostartintervalid);

    $(".managebutton").prop('disabled',false);
    $(".createbutton").prop('disabled',false);
    $(".uploadbutton").prop('disabled',false);
    $(".startbutton").prop('disabled',false);
    $(".syncstartbutton").prop('disabled',false);
    $(".quicksyncbutton").prop('disabled',false);
    $(".cachesyncbutton").prop('disabled',false);
    $(".gpartedbutton").prop('disabled',false);
    
}


function adminelementsvisible() {
 //   $("#adminpanel").show();

    $(".managebutton").show();
    $(".createbutton").show();
    $(".uploadbutton").show();
//   $(".consolebutton").show();
    $(".vmmanagebutton").show();

 //   $(".gpartedbutton").show();
 //  $(".updatestartconfbutton").show();
 //   $(".linboserverbuttosn").show();
}


function adminelementsinvisible() {
 //   $("#adminpanel").hide();

    $(".managebutton").hide();
    $(".createbutton").hide();
    $(".uploadbutton").hide();
//    $(".consolebutton").hide();
    $(".vmmanagebutton").hide();

//    $(".gpartedbutton").hide();
//    $(".updatestartconfbutton").hide();
//    $(".linboserverbutton").hide();
}

function imageSelectedPartitions() {
    // get all inputs
    var docinputs = document.getElementsByTagName('input');
    // get checkboxes
    for( var i=0; i < docinputs.length; i++ ) {
	if( docinputs[i].id == "partitioncheckbox" ) {
	    if( docinputs[i].checked == true ) {

		alert(docinputs[i].value+' checked, targetname '+document.getElementById('label-'+docinputs[i].value).value);

		if( !isEmpty(document.getElementById('label-'+docinputs[i].value).value) ) {		
		    if(  linbocmdisrunning==1 ) {
			alert('linbo_cmd is running...please be patient.');
			return;
		    }

		    linbocmdmkcloop(docinputs[i].value,
				    document.getElementById('label-'+docinputs[i].value).value);
		} else {
		    alert('Imagename is empty or empty string!');
		    return;
		}
		
	    }
	}
    }
}

function uploadSelectedPartitions() {
    // get all inputs
    var docinputs = document.getElementsByTagName('input');
    // get checkboxes
    for( var i=0; i < docinputs.length; i++ ) {
	if( docinputs[i].id == "partitioncheckbox" ) {
	    if( docinputs[i].checked == true ) {
		alert(docinputs[i].value+' checked, targetname '+document.getElementById('label-'+docinputs[i].value).value);
		if( !isEmpty(document.getElementById('label-'+docinputs[i].value).value) ) {
		    if(  linbocmdisrunning==1 ) {
			alert('linbo_cmd is running...please be patient.');
			return;
		    }
		    linbocmdupload(document.getElementById('label-'+docinputs[i].value).value);
		} else {
		    alert('Imagename is empty or empty string!');
		    return;
		}
	    }
	}
    }
}

$(document).ready(documentisready());
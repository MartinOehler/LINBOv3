// **************************************************************************
// Wizard
// **************************************************************************

function createClientFromTemplate() {

    var clientgroupselection = document.getElementById('clientgroupselector').value;
    var clienttemplateselection = document.getElementById('templateselector').value;
    var ip = document.getElementById('newclientip').value;

    Dajaxice.linboweb.linboserver.createClientFromTemplate(Dajax.process,
							   {'clientgroupselection':clientgroupselection,
							    'clienttemplateselection':clienttemplateselection,
							    'ip':ip});

    // update view
    // display wizard
    // setTimeout( function(){Dajaxice.linboweb.linboserver.displayeditor(Dajax.process, {'option':selection});}, 500 );							   
 
    return(false);
};
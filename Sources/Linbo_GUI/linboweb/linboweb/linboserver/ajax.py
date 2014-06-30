from dajax.core import Dajax
from dajaxice.core import dajaxice_functions
from dajaxice.utils import deserialize_form

from django.template.loader import render_to_string

# decorators
from dajaxice.decorators import dajaxice_register
from django.contrib.auth.decorators import login_required

# to use dajax, we need to render the views in a request context
from django.template import RequestContext

from linboweb.linboserver.models import partition
from linboweb.linboserver.models import partitionSelection
from linboweb.linboserver.models import vg
from linboweb.linboserver.models import lv
from linboweb.linboserver.models import os
from linboweb.linboserver.models import vm
from linboweb.linboserver.models import client

from linboweb.linboserver.forms import partitionForm
from linboweb.linboserver.forms import partitionSelectionForm
from linboweb.linboserver.forms import vgForm
from linboweb.linboserver.forms import lvForm
from linboweb.linboserver.forms import osForm
from linboweb.linboserver.forms import vmForm
from linboweb.linboserver.forms import clientForm

from django.forms.widgets import Select
from django.forms import CharField

# TODO: correct csrf handling
from django.views.decorators.csrf import csrf_exempt
from django.forms.models import modelformset_factory
from django.core.exceptions import ValidationError
from django.contrib.auth.models import User

from django.utils import simplejson

@login_required
@dajaxice_register
def displayeditor(request,
                  option):
  
    dajax = Dajax()
    render=''

    if option == 'Edit Partitions':
        partitionFormset = modelformset_factory(partition,
                                                form=partitionForm,
                                                extra=1)

        partitions = partitionFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_partitionAdmin':partitions,
                                    'partitionAdminVisible':'1'})

    if option == 'Edit Partition Selection':
        partitionSelectionFormset = modelformset_factory(partitionSelection,
                                                         form=partitionSelectionForm,
                                                         extra=1)

        partitionSelections = partitionSelectionFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_partitionSelectionAdmin':partitionSelections,
                                    'partitionSelectionAdminVisible':'1'})

    if option == 'Edit Volume Groups':
        vgFormset = modelformset_factory(vg,
                                         form=vgForm,
                                         extra=1)

        vgs = vgFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_vgAdmin':vgs,
                                    'vgAdminVisible':'1'})

    if option == 'Edit logical Volumes':
        lvFormset = modelformset_factory(lv,
                                         form=lvForm,
                                         extra=1)

        lvs = lvFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_lvAdmin':lvs,
                                    'lvAdminVisible':'1'})

    if option == 'Edit Operating Systems':
        osFormset = modelformset_factory(os,
                                         form=osForm,
                                         extra=1)

        oss = osFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_osAdmin':oss,
                                    'osAdminVisible':'1'})

    if option == 'Edit virtual Machines':
        vmFormset = modelformset_factory(vm,
                                         form=vmForm,
                                         extra=1)

        vms = vmFormset()

        render = render_to_string('editor_div.html', 
                                  { 'formset_vmAdmin':vms,
                                    'vmAdminVisible':'1'})



    dajax.assign('#editor','innerHTML',render)
    # dajax.script('reload();')
    return dajax.json()

@login_required
@dajaxice_register
def displayclienteditor(request):
  
    print "inside ajaxx"
    dajax = Dajax()
    render=''
    dajax.alert('inside');


    clientFormset = modelformset_factory(client,
                                         form=clientForm,
                                         extra=1)

    clients = clientFormset()

    render = render_to_string('client_div.html', 
                              { 'formset_clientAdmin':clients,
                                'clientAdminVisible':'1'})

    dajax.assign('#editor','innerHTML',render)
    # dajax.script('reload();')
    return dajax.json()

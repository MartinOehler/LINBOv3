from django.forms import ModelForm
from django.forms import Form
from django.forms import ModelChoiceField

from django.forms.widgets import RadioSelect
from django.forms.widgets import TextInput
from django.forms.widgets import Textarea
from django.forms.widgets import DateInput
from django.contrib.admin import widgets

from linboweb.linboserver.models import partition
from linboweb.linboserver.models import partitionSelection
from linboweb.linboserver.models import vg
from linboweb.linboserver.models import lv
from linboweb.linboserver.models import os
from linboweb.linboserver.models import vm
from linboweb.linboserver.models import client


class partitionForm(ModelForm):
    class Meta:
        model = partition

class partitionSelectionForm(ModelForm):
    class Meta:
        model = partitionSelection

class vgForm(ModelForm):
    class Meta:
        model = vg

class lvForm(ModelForm):
    class Meta:
        model = lv

class osForm(ModelForm):
    class Meta:
        model = os
        
class vmForm(ModelForm):
    class Meta:
        model = vm

class clientForm(ModelForm):
    class Meta:
        model = client

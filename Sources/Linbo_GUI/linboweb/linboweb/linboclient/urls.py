from django.conf.urls.defaults import *
from linboweb.linboclient.views import main

urlpatterns = patterns('',
    url(r'^$',             main,   name='linboclient_main'),
)

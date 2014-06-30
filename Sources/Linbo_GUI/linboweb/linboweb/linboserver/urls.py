from django.conf.urls.defaults import *
from linboweb.linboserver.views import login_password
from linboweb.linboserver.views import main
from linboweb.linboserver.views import client
from linboweb.linboserver.views import config


urlpatterns = patterns('',
    url(r'^$',             login_password,   name='linboserver_login'),
    url(r'^main/$',        main,  	     name='linboserver_main'),
    url(r'^client/$',      client,  	     name='linboserver_client'),
    url(r'^config/$',      config,  	     name='linboserver_config')
)

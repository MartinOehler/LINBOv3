# Author: Martin Oehler <oehler@knopper.net> 2013
# License: GPL V2

from django.template import loader, Context
from django.http import HttpResponseRedirect
from django.shortcuts import render_to_response
from django.contrib.auth import authenticate, login, logout
from django.contrib.auth.decorators import login_required
from django.contrib.auth.backends import ModelBackend
from django.conf import settings

# TODO: change this later
from django.views.decorators.csrf import csrf_exempt    
from django.core.urlresolvers import reverse
from django.views.decorators.cache import never_cache
from django.contrib.auth.decorators import login_required
from django.contrib.auth.models import User
from django.template import RequestContext
from django.contrib import messages

import os
import string
import errno
from django.core.files import File

@csrf_exempt
@never_cache
def main(request,
         template='linboclient_main.html'):


    return render_to_response('linboclient_main.html',
                              {},
                              context_instance=RequestContext(request))

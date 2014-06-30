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

from linboweb.linboserver.models import client as linboclient
from linboweb.linboserver.models import vg
from linboweb.linboserver.models import lv
from linboweb.linboserver.models import partitionSelection
from linboweb.linboserver.models import partition
from linboweb.linboserver.models import vm

LINBO_USERNAME = getattr(settings, 'LINBO_USERNAME', 'linbouser')

editorlist = [ 'Edit Partition Selection',
               'Edit Partitions',
               'Edit Volume Groups',
               'Edit logical Volumes',
               'Edit Operating Systems',
               'Edit virtual Machines' ]


@csrf_exempt
@never_cache
def login_password(request):
    username = password = ''
    state = 'Please log in'
    if request.POST:
        username = request.POST.get('username')
        password = request.POST.get('password')

        user = authenticate(username=username, password=password)
        if user is not None:
            if user.is_active:
                login(request, user)
                
                # add username to session
                request.session["LINBO_USERNAME"] = username;   

                return HttpResponseRedirect(reverse('linboserver_main'));           
            else:
                state = "Your account is not active, please contact the site admin."
        else:
            state = "Your username and/or password were incorrect. Try again."

    return render_to_response('login.html',{'state':state, 'username': username})

@csrf_exempt
@never_cache
@login_required
def main(request,
         template='main.html'):

    return render_to_response('main.html')
                     
@csrf_exempt
@never_cache
@login_required
def config(request,
         template='config.html'):

    return render_to_response('config.html', 
                              {'editorlist':editorlist},
                              context_instance=RequestContext(request))

@csrf_exempt
@never_cache
@login_required
def client(request,
           template='client.html'):

    editorlist=''

    # open the file
    f=open('/tmp/linbo.conf','w')

    # get all clients configs
    for linbocl in linboclient.objects.all():
        # write header of general LINBO section
        f.write("[LINBO]\n")
        f.write("Cache = " + linbocl.cache + "\n")
        f.write("Server = " + linbocl.server + "\n")
        f.write("Mount = " + linbocl.mount + "\n")
        f.write("RootTimeout = " + str(linbocl.roottimeout) + "\n")
        f.write("AutoPartition = " + str(linbocl.autopartition) + "\n")
        f.write("AutoInitCache  = " + str(linbocl.autoinitcache) + "\n")
        f.write("AutoFormat = " + str(linbocl.server) + "\n")
        f.write("TorrentEnabled = " + str(linbocl.torrentenabled) + "\n\n")

        for osentry in linbocl.osentries.all():
            for vgroup in osentry.volumegroups.all():
                f.write("[VG]\n")
                f.write("Name = " + vgroup.name + "\n")
                f.write("Dev = " + vgroup.dev + "\n\n")
                
                # add try
                logicalvolumes = lv.objects.filter(vgname=vgroup)
                for lvol in logicalvolumes:
                    f.write("[LV]\n")
                    f.write("Name = " + lvol.name + "\n")
                    f.write("VGName = " + vgroup.name + "\n")
                    f.write("Size = " + str(lvol.size) + "\n")
                    f.write("FSType = " + lvol.fstype + "\n\n")


            for pselection in osentry.partitionselections.all():
                for part in pselection.partitions.all():
                    f.write("[Partition]\n")
                    f.write("Osid = " + str(osentry.osid) + "\n")
                    f.write("Dev = " + part.device + "\n")
                    f.write("Size = " + str(part.size) + "\n")
                    f.write("Id = " + str(part.fsid) + "\n")
                    f.write("FSType = " + str(part.fstype) + "\n")
                    f.write("Bootable = " + str(part.bootable) + "\n\n")

            f.write("[OS]\n")
            f.write("Osid = " + str(osentry.osid) + "\n")
            f.write("Name = " + osentry.name + "\n")
            f.write("Version = " + osentry.version + "\n")
            f.write("Description = " + osentry.description + "\n")
            f.write("Image = " + osentry.image + "\n")
            f.write("BaseImage = " + osentry.baseimage + "\n")
            f.write("Boot = " + osentry.boot + "\n")
            f.write("Root = " + osentry.root + "\n")
            f.write("Kernel = " + osentry.kernel + "\n")
            f.write("Initrd = " + osentry.initrd + "\n")
            f.write("Append = " + osentry.append + "\n")
            f.write("StartEnabled = " + str(osentry.startenabled) + "\n")
            f.write("SyncEnabled = " + str(osentry.syncenabled) + "\n")
            f.write("RemoteSyncEnabled = " + str(osentry.remotesyncenabled) + "\n")
            f.write("Patches = " + osentry.patches + "\n\n")

        # TODO: partition entry for cache
        for virtualMachine in linbocl.vms.all():
            f.write("[VM]\n")
            f.write("Name = " + virtualMachine.name + "\n")
            f.write("Patches = " + virtualMachine.patches + "\n")

    f.close()

    return render_to_response('client.html', 
                              {'editorlist':editorlist},
                              context_instance=RequestContext(request))

 
 

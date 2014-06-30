# Author: Martin Oehler <oehler@knopper.net> 2013
# License: GPL V2

from dajax.core import Dajax
from dajaxice.core import dajaxice_functions
from dajaxice.utils import deserialize_form

from django.template.loader import render_to_string

# decorators
from dajaxice.decorators import dajaxice_register
from django.contrib.auth.decorators import login_required

# to use dajax, we need to render the views in a request context
from django.template import RequestContext

# TODO: correct csrf handling
from django.views.decorators.csrf import csrf_exempt
from django.core.exceptions import ValidationError
from django.contrib.auth.models import User

from django.utils import simplejson
import os
import subprocess
import shlex
import re
from subprocess import call
from array import *

# custom button classes
#
# startbutton
# syncbutton
# uploadbutton
# managebutton
# createbutton
server=''
last_linbo_action='none'
logline=0

def checkmastermode():

    retval=1
    try:
        arg='grep -q master /proc/cmdline &>/dev/null'

        args=shlex.split(arg)
        retval=subprocess.check_call(args)
    except subprocess.CalledProcessError,e:
        print "calledprocesserror in checkmastermode"
        print e.cmd
        print e.returncode
        print e.output
        retval=1

    if retval==0:
        # master mode detected
        return 1
    else:
        return 0

    return 0

@dajaxice_register
def displaylog(request):

    dajax = Dajax()
    global last_linbo_action
    global logline
    log=''  
    line=''

    i=0
    print "value of logline = "+str(logline)

    try:
        f=open("/tmp/linbo.log",'r')
        for line in f:
            i=i+1
            if i > logline:
                dajax.script("appendLog('"+line.strip()+"<br>');")
    except IOError:
        print "Can't open /tmp/linbo.log"
    else:
        f.close()
        logline=i
        
    # processing the last line
    
    # linbo is filling up the device with zeros
    if re.match(r"Leeren.*", line):
        last_linbo_action=line.strip("\n")
    
    # linbo is compressing
    if re.match(r"^.*Blk.*ratio.*speed",line):
        m=re.search("[0-9]+%$",line)
        if m:
            last_linbo_action="Kompression: %s" % m.group(0).strip("\n")

    # linbo is uploading
    if re.match(r"^RSYNC.*[0-9]+%$",line):
        m=re.search("[0-9]+%$",line)
        if m:
            last_linbo_action="RSYNC: %s" % m.group(0).strip("\n")
    
#    arg="/usr/bin/linbo_cmd version"
#    args=shlex.split(arg)
#    retval=subprocess.check_output(args,stderr=linbolog)
#    retval=retval.strip()

 #   dajax.assign('#linbolabel','innerHTML',retval)

#    dajax.assign('#log','innerHTML',log)
    return dajax.json()

@dajaxice_register
def cancellinbocmd(request):

    dajax = Dajax()
    retval=-1

    dajax.alert('cancel linbo entered!');
    with open("/tmp/linbo.log","a") as linbolog:
        try:
            arg='/usr/bin/asroot /usr/bin/killall linbo_cmd'
            args=shlex.split(arg)
            subprocess.check_call(args,stdout=linbolog,stderr=linbolog)
            print "retval of killall = "+str(retval)
        except subprocess.CalledProcessError,e:
            print "calledprocesserror in cancellinbocmd"
            print e.cmd
            print e.returncode
            print e.output
        finally:
            linbolog.close()

    return dajax.json()

@dajaxice_register
def linbocmdrunning(request):

    dajax = Dajax()
    retval=-5

    with open("/tmp/linbo.log","a") as linbolog:
        try:
            arg='/usr/bin/pgrep -c linbo_cmd'
            args=shlex.split(arg)

            # we really need the output here, not the return value of pgrep
            # which will execute successful most times
            retval=subprocess.check_output(args,stderr=linbolog)
            print "retval linbocmdrunning = "+str(retval)
        except subprocess.CalledProcessError,e:
            print "calledprocesserror in linbocmdrunning"
            print e.cmd
            print e.returncode
            print e.output
        finally:
            linbolog.close()

        if retval>0:
            dajax.script("linbocmdsetrunning();")
            print "progresspanelsetstatusline('"+last_linbo_action+"');"
            dajax.script("progresspanelsetstatusline('"+last_linbo_action+"');")
        else:
            dajax.script("linbocmdsetnotrunning();")

    return dajax.json()



# important: don't forget to add 
# SESSION_ENGINE = "django.contrib.sessions.backends.cache"
# to settings.py for the database-less operation to work
@dajaxice_register
def linbologin(request,
               username,
               password):

    dajax=Dajax()
    global server
    server=''
    
    uname=request.session.get('username','invalid')

    if uname != 'invalid':
        
        # dajax.alert("You're already logged in, username" + request.session['username'] + " logging out" )

        try:
            del request.session['username']
        finally:
            request.session['username'] = 'invalid'

            # change login button
            dajax.assign('#loginbutton','innerHTML','Login')

            # hide buttons
            dajax.script("adminelementsinvisible();")
            dajax.redirect('http://localhost/linboclient/', delay=0)


    else:
        # dajax.alert("Session variable empty, logging in with " + username + "," + password)
        # server=''
        try:
            with open("/start.conf",'r') as f:
                for line in f:
                    if line.startswith("Server ="):
                        server = line.split(" = ")[1]
                        server.strip("\n")

            f.close()
        except IOError:
            print "Can't open /start.conf"
        else:
            f.close()  


        retval="-5"
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                arg='/usr/bin/rsync '+username+'@'+server.strip()+'::linbo-upload &>/dev/null'
                args=shlex.split(arg)
                os.putenv("RSYNC_PASSWORD",password)
                retval=subprocess.check_call(args,stdout=linbolog,stderr=linbolog)
                print "retval"+str(retval)
            except subprocess.CalledProcessError,e:
                print "calledprocesserror in linbologin"
                print e.cmd
                print e.returncode
                print e.output
            finally:
                linbolog.close()

        if str(retval)=='0':
            request.session['username'] = username
            request.session['password'] = password

            # change login button
            dajax.assign('#loginbutton','innerHTML','Logout')

            # show buttons
            dajax.script("adminelementsvisible();")
            # render info div
            dajax.script("Dajaxice.linboweb.linboclient.loadinfodiv(Dajax.process);")
        else:
            dajax.alert('Access denied')
            
    return dajax.json()

# this div displays the partitioning data and other helpful data
# for adding clients to the database
@dajaxice_register
def  loadinfodiv(request):

    dajax=Dajax()
    info=''
    adminbuttons=''
    cmdstring=''

    uname=request.session.get('username','invalid') 

    if uname == 'invalid':
        # no user is logged in, show warning
        info+='<p>Login required</p>'
        dajax.assign('#infodiv','innerHTML',info)
        dajax.assign('#adminbuttons_main','innerHTML',info)
        dajax.redirect('http://localhost/linboclient/', delay=0)
        return dajax.json()

    # first, gather information from our system
    with open("/tmp/linbo.log","a") as linbolog:
        try:
            cmd1="awk '{print \"/dev/\"$4\" \"$3}' < /proc/partitions"
            p1=subprocess.Popen(cmd1,stderr=linbolog,stdout=subprocess.PIPE, shell=True)
                
            cmd2="grep -E '/dev/[shx]v?d[a-z][0-9]+'"
              
            p2=subprocess.Popen(cmd2,stdin=p1.stdout,stdout=subprocess.PIPE,stderr=linbolog,shell=True) 
            p1.stdout.close()
            r2=p2.communicate()[0]
            print "Result= %s" % r2
            print "Result.split() = %s" % r2.split()

            a2=r2.split()

            # the r2 is now formatted as device size, let's gather the
            # remaining data

            info+='<div class="row">'
            info+='<div class="columns large-7 text-align:center">'
            info+='<h5>Partition Layout</h5>'
            info+='<table>'
            info+='<tr><th>Device</th><th>Size</th><th>Filesystem Type</th><th>Select</th><th>ImageName</th></tr>'
            for i in range(0, len(a2)/2):
                print 2*i, a2[(2*i)]
                print 2*i+1, a2[(2*i+1)]

                cmd3="asroot /sbin/blkid "+a2[(2*i)]+" -o export"
                print cmd3
                p3=subprocess.Popen(cmd3,stderr=linbolog,stdout=subprocess.PIPE, shell=True)

                cmd4="grep '^TYPE'"
                p4=subprocess.Popen(cmd4,stdin=p3.stdout,stdout=subprocess.PIPE,stderr=linbolog,shell=True)
                p3.stdout.close()
                r4=p4.communicate()[0]
                if r4 != '':
                    r4=r4.split("=")[1]

                    print r4
                    info+='<tr><td>'+a2[(2*i)]+'</td>'
                    info+='    <td>'+a2[(2*i+1)]+'</td>'
                    info+='    <td>'+r4+'</td>'
                    info+='    <td><input type="radio" id="partitioncheckbox" name="partitionradio" value="'+a2[(2*i)]+'"></td>'
                    info+='    <td><input type="text" id="label-'+a2[(2*i)]+'"></td></tr>'

            info+='</table>'
            info+='</div>'
            info+='<div class="columns large-2 left">'
            info+='<button type="button" onclick="Dajaxice.linboweb.linboclient.loadinfodiv(Dajax.process);" class="button radius tiny">Recalculate Partition Layout</button>'
            info+='<button type="button"  onclick="linbocmd(\'linbo-smbmount\',\'null\');" class="button radius tiny">Mount SMB Share</button>'
            info+='<button type="button"  onclick="imageSelectedPartitions();" class="button radius tiny">Image Selected Partition</button>'
            info+='<button type="button"  onclick="uploadSelectedPartitions();" class="button radius tiny">Upload Selected Partition</button>'
            info+='</div>'
            info+='</div>'

            dajax.assign('#infodiv','innerHTML',info)

        except:
            dajax.alert('Processing info div failed')
        finally:
            linbolog.close()
            dajax.assign('#infodiv','innerHTML',info)
            
        # gather linbo information
        linboversion=''
        ip=''
        mac=''

        with open("/tmp/linbo.log","a") as linbolog:
            try:
                arg='linbo_cmd version'
                args=shlex.split(arg)

                # we really need the output here, not the return value of linbo_cmd
                retval=subprocess.check_output(args,stderr=linbolog)
                linboversion=retval.strip('\n')
            
            except subprocess.CalledProcessError,e:
                print "calledprocesserror in loadinfodiv"
                print e.cmd
                print e.returncode
                print e.output
                linboversion='error'
            finally:
                linbolog.close()

        with open("/tmp/linbo.log","a") as linbolog:
            try:
                arg='linbo_cmd ip'
                args=shlex.split(arg)

                # we really need the output here, not the return value of linbo_cmd
                retval=subprocess.check_output(args,stderr=linbolog)
                ip=retval.strip('\n')
            
            except subprocess.CalledProcessError,e:
                print "calledprocesserror in loadinfodiv"
                print e.cmd
                print e.returncode
                print e.output
                ip='error'
            finally:
                linbolog.close()

        with open("/tmp/linbo.log","a") as linbolog:
            try:
                arg='linbo_cmd mac'
                args=shlex.split(arg)

                # we really need the output here, not the return value of linbo_cmd
                retval=subprocess.check_output(args,stderr=linbolog)
                mac=retval.strip('\n')
            
            except subprocess.CalledProcessError,e:
                print "calledprocesserror in loadinfodiv"
                print e.cmd
                print e.returncode
                print e.output
                mac='error'
            finally:
                linbolog.close()


        state='<div class="row">'
        state+='<div class="large-2 columns">'
        state+='<p><b>LINBO-Version</b><p>'
        state+='</div>'
        state+='<div class="large-5 columns left">'
        state+='<p>'+linboversion+'<p>'
        state+='</div>'
        state+='</div>'
            
        state+='<div class="row">'
        state+='<div class="large-2 columns">'
        state+='<p><b>Client IP</b><p>'
        state+='</div>'
        state+='<div class="large-5 columns left">'
        state+='<p>'+ip+'<p>'
        state+='</div>'
        state+='</div>'
            
        state+='<div class="row">'
        state+='<div class="large-2 columns">'
        state+='<p><b>Client MAC</b><p>'
        state+='</div>'
        state+='<div class="large-5 columns left">'
        state+='<p>'+mac+'<p>'
        state+='</div>'
        state+='</div>'

        dajax.assign('#statusdiv','innerHTML', state)  

        try:
            # render admin buttons
            adminbuttons='<ul class="button-group even radius">'
            adminbuttons+='<li><button type="button" onclick="linbocmd(\'console\',\'null\');" class="button radius consolebutton" title="Terminal"><i class="fi-align-right linbostyle"><br><p class="linbofontstyle">Terminal</p></i></button></li>'
            adminbuttons+='<li><button type="button" onclick="linbocmd(\'update_startconf\',\'null\');" class="button radius updatestartconfbutton" title="Update Start Conf"><i class="fi-page linbostyle"><br><p class="linbofontstyle">Update start.conf</p></i></button></li>'
            adminbuttons+='<li><button type="button" onclick="linbocmd(\'gparted\',\'null\');" class="button radius gpartedbutton" title="GParted"><i class="fi-save linbostyle"><br><p class="linbofontstyle">GParted</p></i></button></li>'
            adminbuttons+='<li><button type="button" onclick="window.open(\'https://'+server.strip()+'/linboweb/\');" class="button radius linboserverbutton" title="Server"><i class="fi-cloud linbostyle"><br><p class="linbofontstyle">LINBO Server</p></i></button></li>'
            adminbuttons+='<li><button type="button" onclick="linbocmd(\'update_linbo\',\'null\');" class="button radius linboupdatebutton" title="Update"><i class="fi-arrow-up linbostyle"><br><p class="linbofontstyle">Update LINBO</p></i></button></li>'
            adminbuttons+='</ul>'

        except:
            dajax.alert('Processing admin buttons failed')
        finally:
            dajax.assign('#adminbuttons_main','innerHTML', adminbuttons)                
        
    return dajax.json()

# this is called every time the dialogue is loaded and thus reads the login
# state from the session variables
@dajaxice_register
def loadmaindiv(request):

    dajax=Dajax()
    nav=''
    startconf=''
    vm='0'
    lv='0'
    autoStartOS=''
    autoSync=0
    autostarttimeout=0
  
    try:
        with open("/start.conf",'r') as f:
            for line in f:                  
                if line.startswith("[VM"):
                    vm='1'

                if line.startswith("[LV"):
                    lv='1'

                if line.startswith("Name ="):
                     name = line.split(" = ")[1]
                     
                     if lv == '1':
                         lv='0'
                         continue                     

                     if vm == '0':
                         if name != '':
                             nav += '<p>' + name.strip() + '</p>'
                             nav += '<ul class="button-group even radius">'
                             nav += '<li><button type="button" onclick="linbocmd(\'start\',\'' + name.strip('\n') + '\');" class="button radius startbutton" title="Start ' + name.strip('\n') +'"><i class="fi-play-circle linbostyle"><br><p class="linbofontstyle">Start</p></i></button></li>'
                             nav += '<li><button type="button" onclick="linbocmd(\'syncstart\',\'' + name.strip('\n') + '\');" class="button radius syncstartbutton" title="Sync+Start ' + name.strip('\n') +'"><i class="fi-refresh linbostyle"><br><p class="linbofontstyle">Sync+Start</p></i></button></li>'
                             nav += '<li><button type="button" onclick="linbocmd(\'quicksync\',\'' + name.strip('\n') + '\');" class="button radius quicksyncbutton" title="Quicksync+Start ' + name.strip('\n') +'"><i class="fi-refresh linbostyle"><br><p class="linbofontstyle">Quicksync</p></i></button></li>'
                             nav += '<li><button type="button" onclick="linbocmd(\'create\',\'' + name.strip('\n') + '\');" class="button radius createbutton" title="Create ' + name.strip('\n') +'"><i class="fi-record linbostyle"><br><p class="linbofontstyle">Create</p></i></button></li>'     
                             nav += '<li><button type="button" onclick="linbocmd(\'upload\',\'' + name.strip('\n') + '\');" class="button radius uploadbutton" title="Upload ' + name.strip('\n') +'"><i class="fi-upload-cloud linbostyle"><br><p class="linbofontstyle">Upload</p></i> </button></li>'
                             nav += '</ul>'
                     else:
                         vm='0'

                         if name != '':
                             nav += '<p>' + name.strip() + ' (Virtual Machine)</p>'
                             nav += '<ul class="button-group even radius">'
                             nav += '<li><button type="button" onclick="linbocmd(\'startvm\',\'' + name.strip('\n') + '\');" class="button radius startbutton" title="Start VM ' + name.strip('\n') +'"><i class="fi-play-circle linbostyle"><br><p class="linbofontstyle">Start</p></i></button></li>'
                             nav += '<li><button type="button" onclick="linbocmd(\'manage\',\'' + name.strip('\n') + '\');" class="button radius vmmanagebutton" title="Modify ' + name.strip('\n') +'"><i class="fi-layout linbostyle"><br><p class="linbofontstyle">Modify</p></i></button></li>'
                             nav += '<li><button type="button" onclick="linbocmd(\'upload\',\'' + name.strip('\n') + '\');" class="button radius uploadbutton" title="Upload VM' + name.strip('\n') +'"><i class="fi-upload-cloud linbostyle"><br><p class="linbofontstyle">Upload</p></i> </button></li>'
                             nav += '</ul>'

        f.close()
    except IOError:
        print "Can't open /start.conf"
    else:
        f.close()  
        
    dajax.assign('#maindiv','innerHTML',nav)

    # check whether our user is logged in
    uname=request.session.get('username','invalid')

    # first: hide buttons
    dajax.script("adminelementsinvisible();");

    # there is an admin logged in, unhide buttons
    if uname != 'invalid':
        dajax.script("adminelementsvisible();")
        dajax.assign('#loginbutton','innerHTML','Logout')


    # since we need to keep order this code is replicated
    # TODO: cleanup
    retval=-1
    autostart='0'


    with open("/tmp/linbo.log","a") as linbolog:
        try:
            arg='/usr/bin/pgrep -c linbo_cmd'
            args=shlex.split(arg)

            # we really need the output here, not the return value of pgrep
            # which will execute successful most times
            retval=subprocess.check_output(args,stderr=linbolog)

            print "pgrep -c linbo_cmd return value (not output) "+str(retval)

        except subprocess.CalledProcessError,e:
            print "calledprocesserror in loadmaindiv"
            print e.cmd
            print e.returncode
            print e.output
        finally:
            linbolog.close()

        # if a linbo process is running, there is a sync running or any other
        # operation so we can't autoboot.
        if retval>0:
            dajax.script("startlinbocmdinterval();")
            dajax.script("linbocmdsetrunning();")
        else:
            dajax.script("linbocmdsetnotrunning();")

            # check for autoboot configuration
            try:
                # reset variables
                name=''
                vm=0
      
                autoStartOS=''
                autoSync=0
                autoStartTimeout=0
                autostart=0
               
                with open("/start.conf",'r') as f:
                    for line in f:
                        if line.startswith("[OS") or line.startswith("[VM"):
                            if line.startswith("[VM"):
                                vm = 1
                            else:
                                vm = 0
                            continue

                        if line.startswith("AutoStartOS"):
                            autoStartOS = line.split(" = ")[1].strip()
                            continue

                        if line.startswith("AutoSync"):
                            autoSync = line.split(" = ")[1].strip()
                            continue


                        if line.startswith("AutoStartTimeout"):
                            autoStartTimeout = line.split(" = ")[1].strip()
                            continue

                        
                        if line.startswith("Name ="):
                            name = line.split(" = ")[1].strip()

                            if name == autoStartOS:
                                # we have a winner

                                # all necessary parameters are read, we can decide about
                                # autostart

                                autostart='1'
                                if autoSync.lower() == 'yes':
                                    if autoStartTimeout != '0':
                                        print 'schedulelinbocmd("syncstart","'+name+'","'+str(autoStartTimeout.strip())+'");'
                                        dajax.script('schedulelinbocmd("syncstart","'+name+'","'+str(autoStartTimeout.strip())+'");')
                                        break
                                    else:
                                        # our default timeout
                                        print 'schedulelinbocmd("syncstart","'+name+'","30");'
                                        dajax.script('schedulelinbocmd("syncstart","'+name+'","30");')
                                    break
                                else:                             
                                    if autoStartTimeout != '0':
                                        if vm==0:
                                            print 'schedulelinbocmd("start","'+name+'","'+str(autoStartTimeout.strip())+'");'
                                            dajax.script('schedulelinbocmd("start","'+name+'","'+str(autoStartTimeout.strip())+'");')
                                        else:
                                            print 'schedulelinbocmd("startvm","'+name+'","'+str(autoStartTimeout.strip())+'");'
                                            dajax.script('schedulelinbocmd("startvm","'+name+'","'+str(autoStartTimeout.strip())+'");')
                                        break
                                    else:
                                        if vm==0:
                                            # our default timeout
                                            print 'schedulelinbocmd("start","'+name+'","30");'
                                            dajax.script('schedulelinbocmd("start","'+name+'","30");')
                                        else:
                                            print 'schedulelinbocmd("startvm","'+name+'","30");'
                                            dajax.script('schedulelinbocmd("startvm","'+name+'","30");')
                                        break
                                break

                f.close()
            except IOError:
                print "Can't open /start.conf"
            else:
                f.close()  

    if autostart==0:
        print "cancel autostart"
        dajax.script("cancelautostart();")

    return dajax.json()


@dajaxice_register
def shell(request,
          command,
          osname):

    dajax=Dajax()
    cmdstring=""
    global last_linbo_action
    last_linbo_action='linbo_cmd is running...'


    mastermode=checkmastermode()

    if ((command=='start') or (command=='startvm') or (command=='manage') or
            (command=='syncstart') or (command=='quicksync')) and (mastermode==1):
        dajax.alert("Not allowed in master mode")
        return dajax.json()
    

    if command == 'start':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd start "+osname.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "start", osname.strip()],stdout=linbolog,stderr=linbolog)  
            finally:
                linbolog.close()

    if command == 'startvm':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd startvm "+osname.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "startvm", osname.strip()],stdout=linbolog,stderr=linbolog)  
            finally:
                linbolog.close()

    if command == 'manage':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd modifyvm "+osname.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "modifyvm", osname.strip()],stdout=linbolog,stderr=linbolog)  
            finally:
                linbolog.close()


    if command == 'upload':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                uname=request.session.get('username','invalid')
                pw=request.session.get('password','invalid')

                if uname != 'invalid' and pw != 'invalid':
                    linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd upload_images "+osname.strip()+"</p>")
                    linbolog.flush()
                    call(["/usr/bin/linbo_cmd", "upload_images", uname, pw, osname.strip()],stdout=linbolog,stderr=linbolog)  
                else:
                    linbolog.write("<p style=\"color:#8C001A\">Login credentials invalid</p>")
            finally:
                linbolog.close()

    if command == 'create':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd create "+osname.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "create", osname.strip()],stdout=linbolog,stderr=linbolog)  
            finally:
                linbolog.close()

    if command == 'syncstart':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd syncstart "+osname.strip()+" fullsync</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "syncstart", osname.strip(), "fullsync"],stdout=linbolog,stderr=linbolog)
            finally:
                linbolog.close()

    if command == 'quicksync':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd syncstart "+osname.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "syncstart", osname.strip()],stdout=linbolog,stderr=linbolog)
            finally:
                linbolog.close()

    if command == 'console':
        cmdstring="DISPLAY=:0 /usr/bin/uxterm &"
        os.system(cmdstring)

    if command == 'gparted':
        # gparted needs udevadm which is in /sbin
        cmdstring="PATH=$PATH:/sbin DISPLAY=:0 /usr/bin/asroot /usr/sbin/gparted &"
        os.system(cmdstring)

    if command == 'linbo-smbmount':
        cmdstring="DISPLAY=:0 /usr/bin/asroot /usr/bin/linbo-smbmount &"
        os.system(cmdstring)

    if command == 'initcache':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd initcache</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "initcache"],stdout=linbolog,stderr=linbolog)
            finally:
                linbolog.close()

    if command == 'update_startconf':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd update_startconf</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "update_startconf"],stdout=linbolog,stderr=linbolog)
            finally:
                linbolog.close()

    if command == 'update_linbo':
        with open("/tmp/linbo.log","a") as linbolog:
            try:
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd update_linbo</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "update_linbo"],stdout=linbolog,stderr=linbolog)
            finally:
                linbolog.close()

    if command == 'reboot':
        cmdstring="/usr/bin/asroot /sbin/reboot"
        os.system(cmdstring)

    if command == 'shutdown':
        cmdstring="/usr/bin/asroot /sbin/halt"
        os.system(cmdstring)
    
    # dajax.script('reload();')
    return dajax.json()


@dajaxice_register
def mkcloop(request,
            partition,
            imagepath):

    dajax=Dajax()
    cmdstring=""
    global last_linbo_action
    last_linbo_action='linbo_cmd is running...'

    with open("/tmp/linbo.log","a") as linbolog:
        try:
            linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd mkcloop "+partition.strip()+" "+imagepath.strip()+"</p>")
            linbolog.flush()
            call(["/usr/bin/linbo_cmd", "mkcloop", partition.strip(), "/cache/"+imagepath.strip()],stdout=linbolog,stderr=linbolog)  
        finally:
            linbolog.close()

    return dajax.json()

@dajaxice_register
def upload(request,
           imagepath):

    dajax=Dajax()
    cmdstring=""
    global last_linbo_action
    last_linbo_action='linbo_cmd is running...'


    with open("/tmp/linbo.log","a") as linbolog:
        try:
            uname=request.session.get('username','invalid')
            pw=request.session.get('password','invalid')

            if uname != 'invalid' and pw != 'invalid':
                linbolog.write("<p style=\"color:#8C001A\">Executing /usr/bin/linbo_cmd upload  username(hidden) password(hidden) /cache/"+imagepath.strip()+"</p>")
                linbolog.flush()
                call(["/usr/bin/linbo_cmd", "upload", uname, pw, "/cache/"+imagepath.strip()],stdout=linbolog,stderr=linbolog)  
            else:
                linbolog.write("<p style=\"color:#8C001A\">Login credentials invalid</p>")
        finally:
            linbolog.close()

    return dajax.json()

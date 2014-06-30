from django.db import models
from django.contrib import admin

# osid is added during writing of the start.conf
class partition(models.Model):
    device   = models.CharField(max_length=20)
    size     = models.BigIntegerField(default=0)
    fsid     = models.CharField(max_length=2)
    fstype   = models.CharField(max_length=20)
    bootable = models.BooleanField(default=False)

# better take a partition selection for the cache
class partitionSelection(models.Model):
    name       = models.CharField(max_length = 200)
    partitions = models.ManyToManyField(partition)

    def __unicode__(self):
        return self.name;

class vg(models.Model):
    name = models.CharField(max_length = 200)
    dev  = models.CharField(max_length = 800)

class lv(models.Model):
    name   = models.CharField(max_length = 200)
    vgname = models.ManyToManyField(vg)
    size   = models.BigIntegerField(default=0)
    fstype = models.CharField(max_length=20)


# OS Definition
# osid needed for efficient sync+start
class os(models.Model):
    osid              = models.IntegerField(default=0)
    name              = models.CharField(max_length=80)
    version           = models.CharField(max_length=80)
    description       = models.CharField(max_length=200)
    image             = models.CharField(max_length=80)
    baseimage         = models.CharField(max_length=80)
    boot              = models.CharField(max_length=80)
    root              = models.CharField(max_length=80)
    kernel            = models.CharField(max_length=80)
    initrd            = models.CharField(max_length=80)
    append            = models.CharField(max_length=200)
    startenabled      = models.BooleanField(default=False)
    syncenabled       = models.BooleanField(default=False)
    remotesyncenabled = models.BooleanField(default=False)
    iconname          = models.CharField(max_length=80)
    patches           = models.CharField(max_length=200)
    volumegroups      = models.ManyToManyField(vg,blank=True)
    partitionselections = models.ManyToManyField(partitionSelection)

class vm(models.Model):
    name              = models.CharField(max_length=80)
    patches           = models.CharField(max_length=200)


# client will contain a linbo conf
class client(models.Model):
    mac       = models.CharField(max_length=20)
    osentries = models.ManyToManyField(os,blank=True)
    server   = models.CharField(max_length=20)
    cache    = models.CharField(max_length=80)
    mount    = models.CharField(max_length=80)
    roottimeout = models.IntegerField(default=180)
    autopartition = models.BooleanField(default=False)
    autoinitcache = models.BooleanField(default=False)
    autoformat = models.BooleanField(default=False)
    torrentenabled = models.BooleanField(default=False)
    vms = models.ManyToManyField(vm,blank=True)

class clientAdmin(admin.ModelAdmin):
    list_display = ( 'mac', )

class partitionSelectionAdmin(admin.ModelAdmin):
    list_display = ( 'name', )

class partitionAdmin(admin.ModelAdmin):
    list_display = ( 'device', )
    
class lvAdmin(admin.ModelAdmin):
    list_display = ('name', )

class vgAdmin(admin.ModelAdmin):
    list_display = ('name', )

class osAdmin(admin.ModelAdmin):
    list_display = ('name', )

class vmAdmin(admin.ModelAdmin):
    list_display = ('name', )


admin.site.register(client, clientAdmin)
admin.site.register(partition, partitionAdmin)
admin.site.register(partitionSelection, partitionSelectionAdmin)
admin.site.register(lv, lvAdmin)
admin.site.register(vg, vgAdmin)
admin.site.register(os, osAdmin)
admin.site.register(vm, vmAdmin)

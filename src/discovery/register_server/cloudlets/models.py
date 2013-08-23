import datetime
from django.utils.timezone import utc
from django.contrib.auth.models import User
from django.db import models
from django.template.defaultfilters import slugify
from uuid import uuid1


now = datetime.datetime.utcnow().replace(tzinfo=utc)

class Cloudlet(models.Model):
    CLOUDLET_STATUS_RUNNING = 'RUN'
    CLOUDLET_STATUS_TERMINATE = 'TER'
    CLOUDLET_STATUS = (
            ( CLOUDLET_STATUS_RUNNING, 'Running' ),
            ( CLOUDLET_STATUS_TERMINATE, 'Terminate' )
    )
    #user = models.ForeignKey(User)
    #uuid = models.CharField(max_length=36)
    #resource_id = models.CharField(max_length=36, primary_key=True, default=lambda :(str(uuid1())))
    pub_date = models.DateTimeField(default=datetime.datetime.now)
    ip_address = models.CharField(max_length=16)
    status = models.CharField(max_length=3, choices=CLOUDLET_STATUS)
    mod_time = models.DateTimeField(default=datetime.datetime.now)
    longitude = models.DecimalField(max_digits=10, decimal_places=4)
    latitude = models.DecimalField(max_digits=10, decimal_places=4)

    def save(self, *args, **kwargs):
        return super(Cloudlet, self).save(*args, **kwargs)

    def __getitem__(self, item):
        return self.__dict__[item]

    def search_out(self):
        ret_dict = dict()
        ret_dict['latitude'] = self.latitude
        ret_dict['longitude'] = self.longitude
        ret_dict['ip_address'] = self.ip_address
        return ret_dict



from django.conf import settings
from django.contrib.auth.models import User
from django.db import models
from django.utils import timezone
from hashlib import sha1

from ..util.utils import send_template_mail

class Invitation(models.Model):
    class Meta:
        permissions = (
            ('invite_user', 'Can invite new users'),
        )

    email = models.EmailField(max_length=254, unique=True)
    invited_by = models.ForeignKey(User, related_name='invitations_sent')

    # Automatically populated
    token = models.CharField(max_length=40, unique=True)
    created = models.DateTimeField(auto_now_add=True)
    last_sent = models.DateTimeField(null=True)

    def __unicode__(self):
        return self.email

    def save(self, *args, **kwargs):
        if not self.pk:
            self.token = sha1(settings.SECRET_KEY + ' ' +
                    self.email).hexdigest()
        super(Invitation, self).save(*args, **kwargs)

    def send(self, request):
        self.last_sent = timezone.now()
        self.save()
        ctx = {
            'inviter': self.invited_by,
            'token': self.token,
        }
        send_template_mail(request, [self.email], ctx,
                'accounts/invite-email-subject.txt',
                'accounts/invite-email-body.txt')


class UserInfo(models.Model):
    class Meta:
        verbose_name_plural = 'User info'

    user = models.OneToOneField(User, primary_key=True)
    invited_by = models.ForeignKey(User, null=True,
            related_name='invited_users')

    def __unicode__(self):
        return self.user.email

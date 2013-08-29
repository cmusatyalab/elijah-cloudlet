import base64
from django.conf import settings
from django.contrib import messages
from django.contrib.auth import authenticate, login
from django.contrib.auth.decorators import login_required, permission_required
from django.contrib.auth.models import Group, User
from django.core import signing
from django.shortcuts import redirect, render
from django.template.defaultfilters import slugify
from django.utils.http import base36_to_int, int_to_base36

from . import forms
from .models import *
from ..util.utils import send_template_mail
from ..util.views import land_message

@permission_required('archive.invite_user')
def invite_user(request):
    if request.method == 'POST':
        form = forms.InviteForm(request.POST)
        if form.is_valid():
            invitation = form.save(commit=False)
            invitation.invited_by = request.user
            invitation.save()
            invitation.send(request)
            messages.success(request, 'Invitation sent to **%s**.' % invitation.email)
            form = forms.InviteForm()
    else:
        form = forms.InviteForm()
    return render(request, 'accounts/invite.html', {
        'form': form,
    })

def register(request, token):
    import pdb;pdb.set_trace()
    try:
        invitation = Invitation.objects.get(token=token)
    except Invitation.DoesNotExist:
        # User already used their invitation, or we revoked it, or the user
        # made up an invitation code.
        return redirect('cloudlet-login')

    if User.objects.filter(email=invitation.email).exists():
        # The email address in the invitation is already registered.
        invitation.delete()
        return redirect('cloudlet-login')

    if request.method == 'POST':
        form = forms.RegisterForm(request.POST)
        if form.is_valid():
            username = slugify(invitation.email.split('@')[0])
            username = '%.20s-%s' % (username, int_to_base36(invitation.id))
            user = User.objects.create_user(
                username=username,
                email=invitation.email,
                password=form.cleaned_data['password1']
            )
            user.first_name = form.cleaned_data['first_name']
            user.last_name = form.cleaned_data['last_name']
            user.save()
            for group_name in settings.USER_DEFAULT_GROUPS:
                try:
                    user.groups.add(Group.objects.get(name=group_name))
                except Group.DoesNotExist:
                    pass
            UserInfo(
                user=user,
                invited_by=invitation.invited_by,
            ).save()
            invitation.delete()
            auth_user = authenticate(username=user.username,
                    password=form.cleaned_data['password1'])
            if not auth_user or not auth_user.is_active:
                return redirect('cloudlet-login')
            login(request, auth_user)
            return redirect('cloudlet-home')
    else:
        form = forms.RegisterForm()
    return render(request, 'accounts/register.html', {
        'form': form,
    })

@login_required
def user_profile(request):
    form = None
    if request.method == 'POST':
        form = forms.ProfileForm(request.POST)
        if form.is_valid():
            have_msg = False
            for field in 'first_name', 'last_name':
                setattr(request.user, field, form.cleaned_data[field])
            if request.user.email != form.cleaned_data['email']:
                new_email = form.cleaned_data['email']
                # Check for duplicates
                # Note: unindexed column!
                if not User.objects.filter(email=new_email).exists():
                    # Send validation email
                    signer = signing.TimestampSigner(salt='chemail')
                    token = signer.sign(base64.urlsafe_b64encode(
                            int_to_base36(request.user.id) + '|' + new_email))
                    ctx = {
                        'token': token,
                    }
                    send_template_mail(request, [new_email], ctx,
                            'accounts/update-email-subject.txt',
                            'accounts/update-email-body.txt')
                    messages.info(request, 'An email has been sent to **%s**.\n\nFollow its instructions to change your email address.' % new_email)
                    have_msg = True
                else:
                    messages.error(request, 'The requested email address is in use by another account.')
                    have_msg = True
            if not have_msg:
                messages.success(request, 'Your changes have been saved.')
            request.user.save()
            form = None
    if not form:
        initial = {}
        for field in 'first_name', 'last_name', 'email':
            initial[field] = getattr(request.user, field)
        form = forms.ProfileForm(initial=initial)
    return render(request, 'accounts/profile.html', {
        'form': form,
    })

def password_changed(request):
    messages.success(request, 'Your password has been changed.')
    return redirect('cloudlet-profile')

def password_reset_sent(request):
    return land_message(request, 'Email Sent',
            'Please check your email for further instructions.')

def password_reset_complete(request):
    messages.success(request, 'Your password has been reset.')
    return redirect('cloudlet-login')

def change_email(request, token):
    try:
        signer = signing.TimestampSigner(salt='chemail')
        uidb36, new_email = base64.urlsafe_b64decode(str(signer.unsign(token,
                max_age=settings.CHANGE_EMAIL_TIMEOUT))).split('|', 1)
        uid = base36_to_int(uidb36)
        user = User.objects.get(id=uid)
        # Check for duplicates.  Unindexed column!  Racy!
        if not User.objects.filter(email=new_email).exclude(id=uid).exists():
            user.email = new_email
            user.save()
            messages.success(request, 'Your email address has been changed.')
        else:
            messages.error(request, 'The requested email address is in use by another account.')
    except signing.SignatureExpired:
        messages.error(request, 'Your email change request has expired.  Please try again.')
    except (signing.BadSignature, User.DoesNotExist):
        messages.error(request, 'Your email change request failed.  Please try again.')
    return redirect('cloudlet-profile')

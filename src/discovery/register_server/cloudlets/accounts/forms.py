from django import forms
from django.contrib.auth.forms import (AuthenticationForm, SetPasswordForm,
        PasswordChangeForm)
from django.contrib.auth.models import User

from .models import Invitation
from .signals import password_changed

class LoginForm(forms.Form):
    '''Login form that uses email/password rather than username/password.
    It uses a captive AuthenticationForm for the actual authentication.'''

    INVALID_LOGIN = 'Please enter a correct email address and password.  Note that both fields are case-sensitive.'
    AMBIGUOUS_EMAIL = 'Email address is ambiguous.  Please contact an administrator.'

    email = forms.EmailField(
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    password = forms.CharField(
            widget=forms.PasswordInput(attrs={'class': 'input-xlarge'}))

    def __init__(self, request=None, *args, **kwargs):
        super(LoginForm, self).__init__(*args, **kwargs)
        self.request = request
        self.auth_form = None

    def clean(self):
        email = self.cleaned_data.get('email')
        password = self.cleaned_data.get('password')

        if not email or not password:
            return

        try:
            # Unindexed column!
            username = User.objects.get(email=email).username
        except User.DoesNotExist:
            raise forms.ValidationError(self.INVALID_LOGIN)
        except User.MultipleObjectsReturned:
            raise forms.ValidationError(self.AMBIGUOUS_EMAIL)

        self.auth_form = AuthenticationForm(self.request, {
            'username': username,
            'password': password,
        })
        if not self.auth_form.is_valid():
            if self.auth_form.non_field_errors():
                err = self.auth_form.non_field_errors()[0]
            else:
                err = self.auth_form.errors.values()[0]

            if err == self.auth_form.error_messages['invalid_login']:
                err = self.INVALID_LOGIN
            raise forms.ValidationError(err)
        return self.cleaned_data

    def get_user_id(self):
        return self.auth_form.get_user_id()

    def get_user(self):
        return self.auth_form.get_user()


class ProfileForm(forms.Form):
    first_name = forms.CharField(max_length=30)
    last_name = forms.CharField(max_length=30)
    email = forms.EmailField()


class InviteForm(forms.ModelForm):
    class Meta:
        model = Invitation
        fields = ('email',)

    def clean_email(self):
        email = self.cleaned_data['email']
        if (User.objects.filter(email=email).exists() or
                self._meta.model.objects.filter(email=email).exists()):
            # Don't leak information about active accounts
            raise forms.ValidationError('This user has already been invited.')
        return email


class RegisterForm(forms.Form):
    first_name = forms.CharField(max_length=30,
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    last_name = forms.CharField(max_length=30,
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    password1 = forms.CharField(label='Password',
            widget=forms.PasswordInput(attrs={'class': 'input-xlarge'}))
    password2 = forms.CharField(label='Confirm password',
            widget=forms.PasswordInput(attrs={'class': 'input-xlarge'}))

    def clean(self):
        cleaned_data = super(RegisterForm, self).clean()
        pw1 = cleaned_data.get('password1')
        pw2 = cleaned_data.get('password2')
        if pw1 and pw2 and pw1 != pw2:
            self._errors['password2'] = self.error_class(['The passwords do not match.'])
            del cleaned_data['password1']
            del cleaned_data['password2']
        return cleaned_data


class SignalingSetPasswordForm(SetPasswordForm):
    def save(self, *args, **kwargs):
        super(SignalingSetPasswordForm, self).save(*args, **kwargs)
        password_changed.send(sender=self, user=self.user)


class SignalingPasswordChangeForm(PasswordChangeForm):
    def save(self, *args, **kwargs):
        super(SignalingPasswordChangeForm, self).save(*args, **kwargs)
        password_changed.send(sender=self, user=self.user)

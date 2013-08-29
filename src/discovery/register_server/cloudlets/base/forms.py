from django import forms

class ContactForm(forms.Form):
    name = forms.CharField(max_length=100,
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    email = forms.EmailField(
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    inquiry_type = forms.ChoiceField(label='Type of Inquiry', choices=(
        ('General', 'General'),
        ('Question', 'Question'),
        ('Comment', 'Comment'),
        ('Media Inquiry', 'Media Inquiry'),
    ), widget=forms.Select(attrs={'class': 'input-xlarge'}))
    message = forms.CharField(
            widget=forms.Textarea(attrs={'class': 'input-xlarge'}))


class MailingListSubscribeForm(forms.Form):
    '''This form is submitted to Mailman, not to us, so we don't control its
    structure.'''

    email = forms.CharField(
            widget=forms.TextInput(attrs={'class': 'input-xlarge'}))
    digest = forms.CharField(initial='0', widget=forms.HiddenInput)

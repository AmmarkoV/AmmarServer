function validate_email(field,alerttxt)
{
with (field)
  {
  apos=value.indexOf("@");
  dotpos=value.lastIndexOf(".");
  if (apos<1||dotpos-apos<2)
    {alert(alerttxt);return false;}
  else {return true;}
  }
}

function validate_required(field,alerttxt)
{
with (field)
  {
  if (value==null||value==""||value=="your name"||value=="email address")
    {
    alert(alerttxt);return false;
    }
  else
    {
    return true;
    }
  }
}

function validate_form(thisform)
{
with (thisform)
  {
	if (validate_required(name,"Please enter your name!")==false)
  {name.focus();return false;}  
  if (validate_email(email,"Please enter your email!")==false)
  {email.focus();return false;}
	if (validate_required(message,"Please enter your message!")==false)
  {message.focus();return false;}  
  }
}
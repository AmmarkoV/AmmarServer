   // The server's POST parser drops every field that follows an empty one , and a plain form always sends
   // both an empty text box and an untouched file input as empty parts. A disabled control is not submitted
   // at all , so emptying either one out of the body entirely is what keeps the other one intact..
   function trimEmptyFields(form)
   {
     var i, element;
     for (i = 0; i < form.elements.length; i++)
     {
       element = form.elements[i];
       if      (element.type == 'file') { element.disabled = (element.files.length == 0); }
       else if (element.type == 'text') { element.disabled = (element.value.length == 0); }
     }
     return true;
   }

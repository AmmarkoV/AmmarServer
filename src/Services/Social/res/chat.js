   // Which conversation this page is showing - a room , or a private thread. The chat index has neither , so
   // it never polls. The key is what the server expects the value under ( "room" or "u" )..
   function conversationField(id)
   {
     var element = document.getElementById(id);
     return (element == null) ? null : element.value;
   }


   function getMessages()
   {
     var key   = conversationField('ckey');
     var value = conversationField('cvalue');
     if (key == null || value == null) { return; }

     var randomnumber=Math.floor(Math.random()*100000);

     var x = new XMLHttpRequest();
     x.open("GET","chatmessages.html?"+key+"="+encodeURIComponent(value)+"&t="+randomnumber,true); //Second parameter is async
     x.send();

     x.onreadystatechange = function()
     {
         if (x.readyState == 4 && x.status == 200)
         { document.getElementById("chatmessages").innerHTML = x.responseText; } else
         {
          console.log( "Failure Requesting Message Update");
         }
      }
   }


   // The server only parses multipart/form-data POST bodies , so this goes out as FormData , exactly like a
   // real <form enctype="multipart/form-data"> submit would. The session rides in its cookie , and the CSRF
   // token the page was rendered with rides along in the body..
   function sendNewMessage()
    {
        var key   = conversationField('ckey');
        var value = conversationField('cvalue');
        if (key == null || value == null) { return; }

        var messageContent = document.getElementById('text').value;
        if (messageContent.length == 0) { return; }

        var fd = new FormData();
        fd.append('csrf', conversationField('csrf'));
        fd.append(key, value);
        fd.append('text', messageContent);

        var x = new XMLHttpRequest();
        x.open("POST","chatSpeak.html",true);
        x.onreadystatechange = function()
        {
          if (x.readyState == 4)
          {
            if (x.status == 200) { getMessages(); goToEndOfMessages(); }
            else                 { console.log("Failure sending message"); }
          }
        }
        x.send(fd);

        document.getElementById('text').value="";
    }


   // A picture or a sound goes to its own endpoint , as its own multipart body , and lands in the
   // conversation as a message whose body is the tag that shows or plays it..
   function sendNewMedia()
    {
        var key   = conversationField('ckey');
        var value = conversationField('cvalue');
        if (key == null || value == null) { return; }

        var picker = document.getElementById('media');
        if (picker == null || picker.files.length == 0) { return; }

        var fd = new FormData();
        fd.append('csrf', conversationField('csrf'));
        fd.append(key, value);
        fd.append('media', picker.files[0]);

        var x = new XMLHttpRequest();
        x.open("POST","chatMedia.html",true);
        x.onreadystatechange = function()
        {
          if (x.readyState == 4)
          {
            if (x.status == 200) { getMessages(); goToEndOfMessages(); }
            else                 { console.log("Failure sending attachment"); }
          }
        }
        x.send(fd);

        picker.value = "";
    }


   function goToEndOfMessages()
   {
    var objDiv = document.getElementById("chatmessages");
    if (objDiv == null) { return; }
    objDiv.scrollTop = objDiv.scrollHeight;
   }

   setInterval(function(){ getMessages(); } , 3500 );

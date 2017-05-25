
   function httpGet(theUrl)
    { 
     var x = new XMLHttpRequest();
     x.open("GET",theUrl,true); //Second parameter is async
     x.send();
     
     var result="";     
     x.onreadystatechange = function() 
     {
         if (x.readyState == 4 && x.status == 200) 
         { console.log( "Success Requesting " );  } else
         { 
          console.log( "Failure Requesting " + theUrl);    
          console.log( x  );  
         }
        result=x.responseText;
      }

     return result;
    }
  
   function getMessages()
   {
     var randomnumber=Math.floor(Math.random()*100000);
     
     var x = new XMLHttpRequest();
     x.open("GET","chatmessages.html?t="+randomnumber,true); //Second parameter is async
     x.send();
     
     var result;     
     x.onreadystatechange = function() 
     {
         if (x.readyState == 4 && x.status == 200) 
         {document.getElementById("chatmessages").innerHTML = x.responseText + "<span id='end' />" } else
         { 
          console.log( "Failure Requesting Message Update");    
          //console.log( x  );  
         } 
      }

     return result; 
   }


   function goToEndOfMessages()
   {
    var objDiv = document.getElementById("chatmessages");
   objDiv.scrollTop = objDiv.scrollHeight
   }
   
   setInterval(function(){ getMessages(); } , 3500 );
 
   function makeUniqueURLs()
   {  
     var links = document.links;
     var i = links.length;
     //Simple var randomnumber=Math.floor(Math.random()*100000);
     
     while (i--) 
     {
       if (links[i].href.indexOf("javascript") == -1) 
          { 
            //Simple links[i].href = links[i].href+"&t="+randomnumber; 
            if (links[i].href.indexOf("chat.html?") != -1) 
              {
                links[i].href = "javascript:command(\'"+getCommandPartOfURL(links[i].href)+"\');"; 
              }
          }
     } 
   }
   
    function makeFeedVisible(name)
    {
      document.getElementById(name).style.visibility='visible';
    }

    function makeFeedInvisible(name)
    {
      document.getElementById(name).style.visibility='hidden';
    }
 

     function command(theCommand)
     {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("chat.html?"+theCommand+"&t="+randomnumber); 
     } 

     function settingsCommand(theCommand)
     {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("chat.html?"+theCommand+"&t="+randomnumber); 
     } 

 
function updateFormRandomNumber()
    {
       var randomnumber=Math.floor(Math.random()*100000);
       document.getElementById("FormRandomField").value = randomnumber;
    }

 
function allowDrop(ev)
{
 //ev.preventDefault();
  return true;
}

function drag(ev)
{  
  ev.dataTransfer.setData("Text",ev.target.id);
}

function drop(ev)
{
  ev.preventDefault(); 
  var data=ev.dataTransfer.getData("Text");
  ev.target.appendChild(document.getElementById(data));
  return true;
} 

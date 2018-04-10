

   var statusMonitorUpdateDelay = 8000;
   var centerX=51;
   var centerY=54;
   
   var speedMultiplier=0.02; //Normal Speed selected by default 
   
   var deadZoneSize = 6; 
   var deadZoneLeft = centerX-deadZoneSize;
   var deadZoneUp = centerY-deadZoneSize;
   var deadZoneRight = centerX+deadZoneSize;
   var deadZoneBottom = centerY+deadZoneSize;

   var limitLeft = 25;
   var limitUp = 27;
   var limitRight = 75;
   var limitBottom = 78;
 
   var ledX = 5;
   var ledY = 5;
   
   var speakLanguage="english";
   var speakString="Testing My Voice";
   var speakGender="female";



   function updateLanguageForms()
   {
    if (speakGender=="female")   
        {  
         if (speakLanguage=="greek")   {  document.getElementById('voiceFemale').value="Afroditi"; } else
         if (speakLanguage=="english") {  document.getElementById('voiceFemale').value="Susan";    } else
         if (speakLanguage=="swedish") {  document.getElementById('voiceFemale').value="Annika";   } else
         if (speakLanguage=="german")  {  document.getElementById('voiceFemale').value="Katrin";   } else
                                       {  alert('Unknown language : '+speakLanguage); } 
         settingsCommand('voiceFemale='+document.getElementById('voiceFemale').value);
        } else
    if (speakGender=="male")   
        {  
         if (speakLanguage=="greek")   {  document.getElementById('voiceMale').value="Nikos";  }  else
         if (speakLanguage=="english") {  document.getElementById('voiceMale').value="Dave";   }  else
         if (speakLanguage=="swedish") {  document.getElementById('voiceMale').value="Sven";   }  else
         if (speakLanguage=="german")  {  document.getElementById('voiceMale').value="Stefan"; }  else
                                       {  alert('Unknown language : '+speakLanguage); }
         settingsCommand('voiceMale='+document.getElementById('voiceMale').value);
        } else
        {
         alert('Unknown gender : '+speakGender);
        }

   }


   function changeUIColor(str)
   {
     var cleanStr = str.replace("#",""); 
     document.getElementById('LuiBackground').style.backgroundColor=str; 
     //alert('The value to transmit is '+cleanStr);
     settingsCommand('LuiBackgroundSelector='+cleanStr);
   }


   function changeLanguage(str)
   {
     if (str=="greek")   {  speakString="Δοκιμή της φωνής μου";  } else
     if (str=="english") {  speakString="Testing my voice";      } else
     if (str=="swedish") {  speakString="Test av min röst";      } else
     if (str=="german")  {  speakString="Test meiner Stimme";    } 
     speakLanguage=str;
     updateLanguageForms();
   }


   function changeGender(str)
   { 
     if (str=="FEMALE")   { speakGender="female"; } else
     if (str=="MALE")     { speakGender="male";   } else
                          { alert('Unknown Gender : '+str); }
     updateLanguageForms();
     settingsCommand('gender='+str);
   }


   function getCommandPartOfURL(str) 
   {
    return str.split('?')[1];
   }


    function handleVoiceAdvancedSwitch(cb)
    {
       if (cb.checked) { document.getElementById("voiceAdvanced").style.visibility='visible'; } else
                       { document.getElementById("voiceAdvanced").style.visibility='hidden'; }
    }



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
            if (links[i].href.indexOf("commander.html?") != -1) 
              {
                links[i].href = "javascript:command(\'"+getCommandPartOfURL(links[i].href)+"\');"; 
              }
          }
     } 
   }
 
    function refreshMapFeed(name,imageName)
    {
      //command('camera=refreshMap');
      document.getElementById(name).style.visibility='visible';
      var randomnumber=Math.floor(Math.random()*100000);
      document.getElementById("mapImage").src="map.png?t="+randomnumber;
    }

    function refreshTFFeed(name,imageName)
    {
      //command('camera=refreshTF');
      document.getElementById(name).style.visibility='visible';
      var randomnumber=Math.floor(Math.random()*100000);
      document.getElementById("tfTreeImage").src="tf.png?t="+randomnumber;
    }


    function refreshTopFeed(name,imageName)
    {
      command('camera=refreshTop');
      document.getElementById(name).style.visibility='visible';
      var randomnumber=Math.floor(Math.random()*100000);
      document.getElementById("videoFeedTopImage").src="top_image.jpg?t="+randomnumber;
    }


    function refreshBottomFeed(name,imageName)
    {
      command('camera=refreshBottom');
      document.getElementById(name).style.visibility='visible';
      var randomnumber=Math.floor(Math.random()*100000);
      document.getElementById("videoFeedBottomImage").src="base_image.jpg?t="+randomnumber;
    }


    function makeFeedVisible(name)
    {
      document.getElementById(name).style.visibility='visible';
    }

    function makeFeedInvisible(name)
    {
      document.getElementById(name).style.visibility='hidden';
    }

    function makeTurnControlsVisible() 
                               {
                                       document.getElementById("turnLeft").style.visibility='visible';
                                       document.getElementById("turnRight").style.visibility='visible';
                               }

    function makeTurnControlsInvisible() 
                               {
                                       document.getElementById("turnLeft").style.visibility='hidden';
                                       document.getElementById("turnRight").style.visibility='hidden';
                               }

    function handleJoystickActiveSwitch(cb)
    {
       if (cb.checked)
           {
             makeTurnControlsVisible();
           } else
           {
             makeTurnControlsInvisible();
           }
    }

   function httpGet(theUrl)
    {
    var xmlHttp = null;

    xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", theUrl, true ); //Second parameter is async
    xmlHttp.send( null );
    return xmlHttp.responseText;
    }

     function joystickExecute(joyX,joyY)
     {
        var randomnumber=Math.floor(Math.random()*100000);
        if ( (joyX==0) && (joyY==0) ) 
          {
            httpGet("commander.html?body=stop&t="+randomnumber); 
          } else
          {
            httpGet("commander.html?body=joystick&x="+joyX+'&y='+joyY+"&t="+randomnumber); 
          }
     }

    
     function command(theCommand)
     {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("commander.html?"+theCommand+"&t="+randomnumber); 
     } 

     function settingsCommand(theCommand)
     {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("settings.html?"+theCommand+"&t="+randomnumber); 
     } 


function updateStatusMonitor()
{
    var iframe = document.getElementById('statusMonitor');
    //iframe.reload(true);
    iframe.contentDocument.location.reload(true);
    //setTimeout('updateStatusMonitor()', statusMonitorUpdateDelay );
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

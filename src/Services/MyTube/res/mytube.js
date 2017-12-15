var myVideo = document.getElementById("myvideo"); 
var keyboardShortcutsEnabled = 1;
var pageReady = 0;
var videoStarted = 0;

function enableKeyboardShortcuts()
{
  keyboardShortcutsEnabled = 1;
}
function disableKeyboardShortcuts()
{
  keyboardShortcutsEnabled = 0;
}
 
function httpGet(theUrl)
    {
    var xmlHttp = null;
    xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", theUrl, true ); //Second parameter is asyncfile:///home/ammar/Documents/Programming/AmmarServer/src/Services/MyTube/res/player.html

    xmlHttp.send( null );
    return xmlHttp.responseText;
    }file:///home/ammar/Documents/Programming/AmmarServer/src/Services/MyTube/res/player.html



    function command(theCommand)
    {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("/proc?"+theCommand+"&t="+randomnumber); 
    } 
    
    
    function playbackError(theVideo)
    {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("/error?v="+theVideo+"&rnd="+randomnumber); 
    } 

    function goRandomVideo() 
    {
      var randomnumber=Math.floor(Math.random()*100000);
      window.location.href = "/random?t="+randomnumber; 
    } 

    function goFixedVideo(vid) 
    {  
      var randomnumber=Math.floor(Math.random()*100000);
      window.location.href = "/watch?v="+vid+"&rnd="+randomnumber; 
    } 

  document.onkeypress = function (e) 
  {
    if (!keyboardShortcutsEnabled) { return; }

    e = e || window.event;
    
    var keypressed=(e.keyCode || e.which);
    if (  keypressed == 114  ) //r
     {
      goRandomVideo();  
     }
  };

 

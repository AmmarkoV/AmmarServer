
var keyboardShortcutsEnabled = 1;

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
    xmlHttp.open( "GET", theUrl, true ); //Second parameter is async
    xmlHttp.send( null );
    return xmlHttp.responseText;
    }


    function command(theCommand)
    {
       var randomnumber=Math.floor(Math.random()*100000);
       httpGet("/proc?"+theCommand+"&t="+randomnumber); 
    } 

    function goRandomVideo() 
    {
      var randomnumber=Math.floor(Math.random()*100000);
      window.location.href = "/random?t="+randomnumber; 
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

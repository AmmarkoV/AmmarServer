var canvas = document.getElementById('joypadCanvas');
         var context = canvas.getContext('2d');
         var joypadObj = new Image();
         var joystickObj = new Image();
         var lastMouseX = 0;
         var lastMouseY = 0;
         var mouseDown = false;



     function clearCanvas()
     {
       var context = canvas.getContext('2d');
       context.clearRect(0, 0, canvas.width, canvas.height); 
     }
 
      function getMousePos(canvas, evt) 
    {
        var rect = canvas.getBoundingClientRect();
        return {
          x: evt.clientX - rect.left,
          y: evt.clientY - rect.top
        };
      }

      function drawJoystick(canvas, x ,y , imageObj) 
      {
        var context = canvas.getContext('2d'); 
        clearCanvas(); 
        context.drawImage(joypadObj,  0 ,0);
        context.drawImage(joystickObj,  x ,y);
      }

      function drawActiveStatus(canvas, x,y, width , height , color )
      {
        context.beginPath();
        context.fillStyle = color;
        context.fillRect(x, y, width, height);
        context.strokeRect(x, y, width, height);
      }
 

     function joystickCallback(x,y)
     {
        var padding = 30;
        var canvas = document.getElementById('joypadCanvas');
        var context = canvas.getContext('2d');
        var joystickActiveSwitch = document.getElementById("joystickActiveSwitch").checked;
  
          if ( x < limitLeft )   { x = limitLeft; }
          if ( x > limitRight )  { x = limitRight; }
          if ( y < limitUp )     { y = limitUp; }
          if ( y > limitBottom ) { y = limitBottom; } 
          
          lastMouseX=x;
          lastMouseY=y;

          drawJoystick(canvas, x,y, joystickObj );
           
          if ( 
               ( ( x <= deadZoneLeft )||( x >= deadZoneRight ) ) ||
                ( ( y <= deadZoneUp )||( y >= deadZoneBottom ) ) 
             )    

          {
          
           speedMultiplier = parseFloat( document.getElementById("joystickSpeed").value );
           //alert('SpeedMultiplier '+speedMultiplier);
           var joyX = (x - centerX) * speedMultiplier;
           var joyY = (y - centerY) * speedMultiplier;
           
           joyX = (-1) * joyX;
           joyY = (-1) * joyY;

           if ( (mouseDown) && (joystickActiveSwitch) )
              {
                 //alert('Mouse '+x+','+y+'\n Joy : '+joyX+','+joyY);   
                 drawActiveStatus(canvas, ledX, ledY , ledX+5 , ledY+5 , "rgb(255,0,0)" ); 
                 joystickExecute(joyX,joyY);
                 mouseDown=false; 
              } else
              {   
                 drawActiveStatus(canvas, ledX, ledY , ledX+5 , ledY+5 , "rgb(0,255,0)" );
              } 
          } else
          {
                 drawActiveStatus(canvas, ledX, ledY , ledX+5 , ledY+5 , "rgb(0,0,255)" );
           if ( (mouseDown)&&(joystickActiveSwitch) )
              {
                 joystickExecute(0,0);
              }
                 mouseDown=false; 
          }
 
     }







      function init() 
    { 
        canvas.addEventListener('mousedown', function() { mouseDown = true; joystickCallback(lastMouseX,lastMouseY); }, false);
        canvas.addEventListener('mouseup', function() { mouseDown = false; }, false); 
        canvas.addEventListener('mousemove', function(evt) 
                                               { 
                                                 var mousePos = getMousePos(canvas, evt);  
                                                 joystickCallback(mousePos.x,mousePos.y); 
                                               }, false); 
    }


       joypadObj.onload = function() { context.drawImage(joypadObj, 0, 0); };
       joypadObj.src = 'joypad.png';


       joystickObj.onload = function() { context.drawImage(joystickObj, 52, 52); };
       joystickObj.src = 'joybtn.png';

       init();  

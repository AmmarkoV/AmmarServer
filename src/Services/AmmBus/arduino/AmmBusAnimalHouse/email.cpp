#include "email.h"


// -----------------------------------------------------------------------------
// Private method that waits until a response is detected or timeout
// -----------------------------------------------------------------------------
int waitForResponse(EthernetClient * client) {

  unsigned long endTime = millis() + 10000;
  while (!client->available()) {
    delay(1);

    // if nothing received for 10 seconds, timeout
    if (millis() > endTime) {
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  // We've detected a response within the time limit
  return 1;
}

// -----------------------------------------------------------------------------
// Private method to display all available response messages from mail server
// -----------------------------------------------------------------------------
void displayResponse(EthernetClient * client) {
  while (client->available()) {
    char thisByte = client->read();
    Serial.write(thisByte);
  }
} 


// -----------------------------------------------------------------------------
// Wait for a response (but not forever) and display it to the debug window
// -----------------------------------------------------------------------------
byte getResponse(EthernetClient * client) {
  if (waitForResponse(client) == 0) {
    client->stop();
    return 0;
  }

  // Have a sneak peek at the response code (next character available) (without actually READing it)
  char respCode = client->peek();

  // Display all the server response text to the debug window
  displayResponse(client);

  // If the response from the client was not good then QUIT and end this session
  // This is the first character of, say, 250 or 500
  if (respCode >= '4') {
    Serial.println(F("Error detected. Attempting to QUIT session..."));
    client->println(F("QUIT"));
    if (waitForResponse(client)) displayResponse(client);
    client->stop();
    Serial.println(F("Disconnected."));
    return 0;
  }

  // All good so far
  return 1;
}

// -----------------------------------------------------------------------------
// SEND SMTP EMAIL      SEND SMTP EMAIL      SEND SMTP EMAIL     SEND SMTP EMAIL
// -----------------------------------------------------------------------------
byte sendEmail(
                EthernetClient * client,
                const char * mailServer,
                const char * fromAddress,
                const char * toAddress
              ) 
{

  // Try and connect to your mail server. If this fails, prove your connection with TelNet.
  Serial.println(F("Connecting to server..."));
  if (client->connect(mailServer, 25)) 
  {
    Serial.println(F("Connected"));
  }
  else 
  {
    Serial.println(F("Connection failed"));
    return 0;
  }

  // Wait for the stuff that your ISP sends back about not spamming people
  if (!getResponse(client)) return 0;

  // Say HELO (=HELLO) or EHLO (Extended HELO) to your mail server (optionally with your own IP address)
  Serial.println(F("HELO"));
  client->println(F("HELO"));
  if (!getResponse(client)) return 0;

  // Tell the mail server who you are - the FROM address (must be valid email address or it will get rejected)
  Serial.println(F("MAIL To: ralph"));
  client->println(fromAddress);
  if (!getResponse(client)) return 0;

  // Declare you who want to send it TO
  Serial.println(F("RCPT To: Arduino"));
  client->println(toAddress);
  if (!getResponse(client)) return 0;

  // Now tell the server you want to send the body of the email
  Serial.println(F("Sending DATA"));
  client->println(F("DATA"));
  if (!getResponse(client)) return 0;

  // This is all just text really. Nothing is validated. Hence spam mails!
  Serial.println(F("Sending email"));

  // How the TO address appears in the email
  client->println(F("To: Ralph S Bacon (yes him from YouTube)"));

  // How is sending this email?
  client->println(F("From: Arduino (Your Best Friend)"));

  // SUBJECT of the email
  client->println(F("Subject: Attic water level alert"));

  // DATE/TIME it was sent (we will need to use NTP/UDP to get this
  client->println(F("Date: Monday 31st February 2040 23:69:64 GMT"));

  // The BODY of the email (all plain text, don't try and fancy HTML here)
  client->println(F("Water level is too cold in header tank: 4 degrees Centigrade."));

  // This IMPORTANT line tells the server you've finished the body of the email
  client->println(F("."));
  if (!getResponse(client)) return 0;

  // Now sign off from the mail server session
  Serial.println(F("Sending QUIT"));
  client->println(F("QUIT"));
  if (!getResponse(client)) return 0;

  // Terminate the client here
  client->stop();

  // And we're all done - success!
  Serial.println(F("disconnected"));
  return 1;
}

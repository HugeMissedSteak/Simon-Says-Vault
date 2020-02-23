/*
 Simon Says is a memory game. Start the game by pressing one of the four buttons. When a button lights up, 
 press the button, repeating the sequence. The sequence will get longer and longer. The game is won after 
 13 rounds.

 Generates random sequence, plays music, and displays button lights.

 Simon tones from Wikipedia
 - A (red, upper left) - 440Hz - 2.272ms - 1.136ms pulse
 - a (green, upper right, an octave higher than A) - 880Hz - 1.136ms,
 0.568ms pulse
 - D (blue, lower left, a perfect fourth higher than the upper left) 
 587.33Hz - 1.702ms - 0.851ms pulse
 - G (yellow, lower right, a perfect fourth higher than the lower left) - 
 784Hz - 1.276ms - 0.638ms pulse

 Simon Says game originally written in C for the PIC16F88.
 Ported for the ATmega168, then ATmega328, then Arduino 1.0.
 Fixes and cleanup by Joshua Neal <joshua[at]trochotron.com>

 This sketch was written by SparkFun Electronics,
 with lots of help from the Arduino community.
 This code is completely free for any use.
 Visit http://www.arduino.cc to learn about the Arduino.
 */

/*************************************************
* Public Constants
*************************************************/

#include "notes.h"
#include "Arduino.h"
#include <LowPower.h>
#include <Keypad.h>

#define CHOICE_NONE   '\000'
#define CHOICE_RED    '0'
#define CHOICE_GREEN  '9'
#define CHOICE_BLUE   '7'
#define CHOICE_WHITE  '5'

#define LED_RED     9
#define LED_GREEN   5
#define LED_BLUE    3

#define UNLOCK_PIN     8
#define UNLOCK_BUTTON  7

// Buzzer pin definitions
#define BUZZER  6

// Define game parameters
#define ROUNDS_TO_WIN      7 //Number of rounds to succesfully remember before you win.
#define ENTRY_TIME_LIMIT   3000 //Amount of time to press a button before game times out. 3000ms = 3 sec

#define MODE_MEMORY  0

// Game state variables
byte gameBoard[32]; //Contains the combination of buttons as we advance
byte gameRound = 0; //Counts the number of succesful rounds the player has made it through

const byte ROWS = 4; //In this two lines we define how many rows and columns
const byte COLS = 3; //our keypad has
char keys[ROWS][COLS] = { //The characters on the keys are defined here
    { '2', '1', '3' },
    { '5', '4', '6' },
    { '8', '7', '9' },
    { '0', 'C', 'E' }
};

byte rowPins[ROWS] = {11, 12, 14, 16}; //The connection with the arduino is
byte colPins[COLS] = {13, 10, 15};    //listed here
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

boolean play_memory(void);
void playMoves(void);
void add_to_moves(void);
void setColor(int red, int green, int blue);
char wait_for_button(void);
void toner(char which, int buzz_length_ms);
void buzz_sound(int buzz_length_ms, int buzz_delay_us);
void play_winner(void);
void winner_sound(void);
void play_loser(void);
void attractMode(void);
void manualUnlock(void);

void setup()
{
  #ifdef SERIAL_ON
  Serial.begin(9600);
  #endif

  //Setup hardware inputs/outputs. These pins are defined in the hardware_versions header file
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(UNLOCK_PIN, OUTPUT);

  pinMode(UNLOCK_BUTTON, INPUT_PULLUP);
  
  digitalWrite(UNLOCK_PIN, LOW);
  digitalWrite(BUZZER, LOW);
}

void loop()
{
  attractMode(); // Blink lights while waiting for user to press a button

  Serial.println("Game mode");

  // Indicate the start of game play
  //setColor(255,255,255); // White
  //delay(1000);
  setColor(0,0,0); // Turn off LED
  delay(200);

  // Play memory game and handle result
  if (play_memory() == true) 
    play_winner(); // Player won, play winner tones
  else 
    play_loser(); // Player lost, play loser tones
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions are related to game play only

// Play the regular memory game
// Returns 0 if player loses, or 1 if player wins
boolean play_memory(void)
{
  Serial.println("play_memory");

  randomSeed(millis()); // Seed the random generator with random amount of millis()

  gameRound = 0; // Reset the game to the beginning

  while (gameRound < ROUNDS_TO_WIN) 
  {
    add_to_moves(); // Add a button to the current moves, then play them back

    playMoves(); // Play back the current game board

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++)
    {
      char choice = wait_for_button(); // See what button the user presses

      if (choice == CHOICE_NONE) return false; // If wait timed out, player loses

      if (choice != gameBoard[currentMove]) return false; // If the choice is incorect, player loses
    }

    delay(1000); // Player was correct, delay before playing moves
  }

  return true; // Player made it through all the rounds to win!
}

// Plays the current contents of the game moves
void playMoves(void)
{
  for (byte currentMove = 0 ; currentMove < gameRound ; currentMove++) 
  {
    toner(gameBoard[currentMove], 150);

    // Wait some amount of time between button playback
    // Shorten this to make game harder
    delay(150); // 150 works well. 75 gets fast.
  }
}

// Adds a new random button to the game sequence, by sampling the timer
void add_to_moves(void)
{
  char newButton = random(0, 4); //min (included), max (exluded)

  // We have to convert this number, 0 to 3, to CHOICEs
  if(newButton == 0) newButton = CHOICE_RED;
  else if(newButton == 1) newButton = CHOICE_GREEN;
  else if(newButton == 2) newButton = CHOICE_BLUE;
  else if(newButton == 3) newButton = CHOICE_WHITE;

  gameBoard[gameRound++] = newButton; // Add this new button to the game array
}

void setColor(int red, int green, int blue)
{
  analogWrite(LED_RED, red);
  analogWrite(LED_GREEN, green);
  analogWrite(LED_BLUE, blue);
}

// Wait for a button to be pressed. 
// Returns one of LED colors (LED_RED, etc.) if successful, 0 if timed out
char wait_for_button(void)
{
  long startTime = millis(); // Remember the time we started the this loop

  while ( (millis() - startTime) < ENTRY_TIME_LIMIT) // Loop until too much time has passed
  {
    char button = keypad.getKey();
    
    if (button != CHOICE_NONE)
    { 
      Serial.println(button);

      toner(button, 150); // Play the button the user just pressed

      while(keypad.getKey() != CHOICE_NONE) ;  // Now let's wait for user to release button

      delay(10); // This helps with debouncing and accidental double taps

      return button;
    }

  }

  return CHOICE_NONE; // If we get here, we've timed out!
}

// Light an LED and play tone
// Red, upper left:     440Hz - 2.272ms - 1.136ms pulse
// Green, upper right:  880Hz - 1.136ms - 0.568ms pulse
// Blue, lower left:    587.33Hz - 1.702ms - 0.851ms pulse
// Yellow, lower right: 784Hz - 1.276ms - 0.638ms pulse
void toner(char which, int buzz_length_ms)
{
  //Play the sound associated with the given LED
  switch(which) 
  {
  case CHOICE_RED:
    setColor(255,0,0);
    buzz_sound(buzz_length_ms, 1136); 
    break;
  case CHOICE_GREEN:
    setColor(0,255,0);
    buzz_sound(buzz_length_ms, 568); 
    break;
  case CHOICE_BLUE:
    setColor(0,0,255);
    buzz_sound(buzz_length_ms, 851); 
    break;
  case CHOICE_WHITE:
    setColor(255,255,255);
    buzz_sound(buzz_length_ms, 638); 
    break;
  }

  setColor(0,0,0); // Turn off all LEDs
}

// Toggle buzzer every buzz_delay_us, for a duration of buzz_length_ms.
void buzz_sound(int buzz_length_ms, int buzz_delay_us)
{
  // Convert total play time from milliseconds to microseconds
  long buzz_length_us = buzz_length_ms * (long)1000;

  // Loop until the remaining play time is less than a single buzz_delay_us
  while (buzz_length_us > (buzz_delay_us * 2))
  {
    buzz_length_us -= buzz_delay_us * 2; //Decrease the remaining play time

    // Toggle the buzzer at various speeds
    digitalWrite(BUZZER, LOW);
    delayMicroseconds(buzz_delay_us);

    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(buzz_delay_us);
  }
}

// Play the winner sound and lights
void play_winner(void)
{
  digitalWrite(UNLOCK_PIN, HIGH);

  setColor(0, 255, 255);
  winner_sound();
  setColor(255, 0, 255);
  winner_sound();
  setColor(255, 255, 0);
  winner_sound();
  setColor(0, 255, 0);
  winner_sound();

  delay(15000);
  digitalWrite(UNLOCK_PIN, LOW);
}

// Play the winner sound
// This is just a unique (annoying) sound we came up with, there is no magic to it
void winner_sound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250 ; x > 70 ; x--)
  {
    for (byte y = 0 ; y < 3 ; y++)
    {
      digitalWrite(BUZZER, HIGH);
      delayMicroseconds(x);

      digitalWrite(BUZZER, LOW);
      delayMicroseconds(x);
    }
  }
}

// Play the loser sound/lights
void play_loser(void)
{
  setColor(255, 0, 0);
  buzz_sound(255, 1500);

  setColor(255, 255, 0);
  buzz_sound(255, 1500);

  setColor(0, 255, 255);
  buzz_sound(255, 1500);

  setColor(255, 0, 255);
  buzz_sound(255, 1500);

  setColor(255, 0, 0);
  delay(3000);
}

// Show an "attract mode" display while waiting for user to press button.
void attractMode(void)
{
  Serial.println("attractMode");
  setColor(0, 0, 0);

  while(1) 
  {
    keypad.getKey();
    delay(10);
    if (keypad.getKey() != CHOICE_NONE) return;
    if (!digitalRead(UNLOCK_BUTTON)) manualUnlock();

    LowPower.idle(SLEEP_250MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
  }
}

void manualUnlock(void){
  digitalWrite(UNLOCK_PIN, HIGH);
  setColor(0,255,0);
  delay(7000);
  digitalWrite(UNLOCK_PIN, LOW);
  setColor(0,0,0);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// The following functions are related to Beegees Easter Egg only

// Notes in the melody. Each note is about an 1/8th note, "0"s are rests.
int melody[] = {
  NOTE_G4, NOTE_A4, 0, NOTE_C5, 0, 0, NOTE_G4, 0, 0, 0,
  NOTE_E4, 0, NOTE_D4, NOTE_E4, NOTE_G4, 0,
  NOTE_D4, NOTE_E4, 0, NOTE_G4, 0, 0,
  NOTE_D4, 0, NOTE_E4, 0, NOTE_G4, 0, NOTE_A4, 0, NOTE_C5, 0};

int noteDuration = 115; // This essentially sets the tempo, 115 is just about right for a disco groove :)
int LEDnumber = 0; // Keeps track of which LED we are on during the beegees loop


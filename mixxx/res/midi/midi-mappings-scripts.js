// Functions common to all controllers go in this file

nop = function () {}    // Only here so you don't get a syntax error on load if the file was otherwise empty

// ----------------- Prototype enhancements ---------------------

// Returns an ASCII byte array for the string
String.prototype.toInt = function() {
    var a = new Array();
    for (var i = 0; i < this.length; i++) {
        a[i] = this.charCodeAt(i);
    }
    return a;
}

// ----------------- Function overloads ---------------------

// Causes script print() calls to appear in the log file as well
print = function(string) {
	engine.log(string);
}

// ----------------- Generic functions ---------------------

function secondstominutes(secs)
{
   var m = (secs / 60) | 0;

   return (m < 10 ? "0" + m : m) 
          + ":"
          + ( ( secs %= 60 ) < 10 ? "0" + secs : secs);
}

function msecondstominutes(msecs)
{
    var m = (msecs / 60000) | 0;
    msecs %= 60000;
    var secs = (msecs / 1000) | 0;
    msecs %= 1000;
    msecs = Math.round(msecs * 100 / 1000);
    if (msecs==100) msecs=99;
    
//     print("secs="+secs+", msecs="+msecs);

    return (m < 10 ? "0" + m : m) 
        + ":"
        + ( secs < 10 ? "0" + secs : secs )
        + "."
        + ( msecs < 10 ? "0" + msecs : msecs);
}

function script() {}
script.debug = function (channel, control, value, status, group) {
    print("Script.Debug --- channel: " + channel.toString(16) + 
          " control: " + control.toString(16) + " value: " + value.toString(16) + 
          " status: " + status.toString(16) + " group: " + group);
}

// Used to control a generic Mixxx control setting (low..high) from an absolute control (0..127)
script.absoluteSlider = function (group, key, value, low, high) {
    if (value==127) engine.setValue(group, key, high);
    else engine.setValue(group, key, (((high - low) / 127) * value) + low);
}

// Returns a value for a non-linear Mixxx control (like EQs: 0..1..4) from an absolute control (0..127)
script.absoluteNonLin = function (value, low, mid, high) {
    if (value<=64) return value/(64/(mid-low));
    else return 1+(value-63)/(64/(high-mid));
}

// DEPRECATED
// Used to control an EQ setting (0..1..4) from an absolute control (0..127)
script.absoluteEQ = function (group, key, value) {
    if (value<=64) engine.setValue(group, key, value/64);
    else engine.setValue(group, key, 1+(value-63)/(21+1/3));
    print ("MIDI Script: script.absoluteEQ is deprecated. " + 
           "Use script.absoluteNonLin(value,0,1,4) instead and set the " + 
           "MixxxControl to its return value.");
}

/* -------- ------------------------------------------------------
     script.Pitch
   Purpose: Takes the value from a little-endian 14-bit MIDI pitch
            wheel message and returns the value for a "rate" (pitch
            slider) Mixxx control
   Input:   Least significant byte, most sig. byte, MIDI status byte
   Output:  Value for a "rate" control, or false if the input MIDI
            message was not a Pitch message (0xE#)
   -------- ------------------------------------------------------ */
script.pitch = function (LSB, MSB, status) {
    if ((status & 0xF0) != 0xE0) {  // Mask the upper nybble so we can check the opcode regardless of the channel
        print("Script.Pitch: Error, not a MIDI pitch message: "+status);
        return false;
    }
    var value = (MSB << 7) | LSB;  // Construct the 14-bit number
    // Range is 0x0000..0x3FFF center @ 0x2000, i.e. 0..16383 center @ 8192
    var rate = (value-8192)/8191;
//     print("Script.Pitch: MSB="+MSB+", LSB="+LSB+", value="+value+", rate="+rate);
    return rate;
}

// bpm - Used for tapping the desired BPM for a deck
function bpm() {}

bpm.tapTime = 0.0;
bpm.tap = [];   // Tap sample values

/* -------- ------------------------------------------------------
        bpm.tapButton
   Purpose: Sets the bpm of the track on a deck by tapping the beats.
            This only works if the track's original BPM value is correct.
            Call this each time the tap button is pressed.
   Input:   Mixxx deck to adjust
   Output:  -
   -------- ------------------------------------------------------ */
bpm.tapButton = function(deck) {
    var now = new Date()/1000;   // Current time in seconds
    var tapDelta = now - bpm.tapTime;
    bpm.tapTime=now;
    if (tapDelta>2.0) { // reset if longer than two seconds between taps
        bpm.tap=[];
        return;
    }
    bpm.tap.push(60/tapDelta);
    if (bpm.tap.length>8) bpm.tap.shift();  // Keep the last 8 samples for averaging
    var sum = 0;
    for (i=0; i<bpm.tap.length; i++) {
        sum += bpm.tap[i];
    }
    var average = sum/bpm.tap.length;
    
    var fRateScale = average/engine.getValue("[Channel"+deck+"]","bpm");
    
    // Adjust the rate:
    fRateScale = (fRateScale-1.)/engine.getValue("[Channel"+deck+"]","rateRange");
    
    engine.setValue("[Channel"+deck+"]","rate",fRateScale * engine.getValue("[Channel"+deck+"]","rate_dir"));
//     print("Script: BPM="+average);
}


// ----------------- DEPRECATED Scratching functions ---------------------
function scratch() {}
// Allows for smooth scratching with MIDI controllers
// See full details here: http://mixxx.org/wiki/doku.php/midi_scripting#available_common_functions

// ----------   Variables    ----------
scratch.variables = { "time":0.0, "trackPos":0.0, "initialTrackPos":-1.0,
                      "initialControlValue":0, "scratch":0.0,
                      "prevControlValue":0, "wrapCount":0 };

// ----------   Functions   ----------

/* -------- ------------------------------------------------------
    scratch.Enable
   Purpose: Sets up initial variables for the scratch.<control>
            functions below. Called when a modifier button is pressed
            & held.
   Input:   Currently-controlled Mixxx deck
   Output:  -
   -------- ------------------------------------------------------ */
scratch.enable = function (currentDeck,newBehavior) {
    // Store scratch info at the point it was touched
    // Current position in seconds:
    scratch.variables["initialTrackPos"] = scratch.variables["trackPos"] = engine.getValue("[Channel"+currentDeck+"]","visual_playposition") * engine.getValue("[Channel"+currentDeck+"]","duration");
    
    scratch.variables["time"] = new Date()/1000;   // Current time in seconds
    if (newBehavior) engine.setValue("[Channel"+currentDeck+"]","scratch2_enable", 1);
    
    // If the deck is playing, slow it to a stop
    if (engine.getValue("[Channel"+currentDeck+"]","play") > 0) {
        // TODO: ramp down
    }
    
//     print("MIDI Script: Scratch initial: time=" + scratch.variables["time"] + "s, track=" + scratch.variables["trackPos"] + "s");
    return;
}

/* -------- ------------------------------------------------------
    scratch.Disable
   Purpose: Clears varaibles used by the scratch.<control> functions
            below. Called when the modifier button is released.
   Input:   Currently-controlled Mixxx deck
   Output:  -
   -------- ------------------------------------------------------ */
scratch.disable = function (currentDeck) {
    // If the deck is playing, ramp it up to the play speed
    if (engine.getValue("[Channel"+currentDeck+"]","play") > 0) {
        //TODO: ramp up
    }
    // Reset the triggers
    scratch.variables["trackPos"] = 0.0;
    scratch.variables["initialTrackPos"] = -1.0;
    scratch.variables["initialControlValue"] = 0;
    scratch.variables["prevControlValue"] = 0;  // for wheel
    scratch.variables["wrapCount"] = 0; // for wheel
    scratch.variables["time"] = 0.0;
    scratch.variables["scratch"] = 0.0;
//     print("MIDI Script: Scratch values CLEARED");
    engine.setValue("[Channel"+currentDeck+"]","scratch2_enable", 0);  // disable scratching
    engine.setValue("[Channel"+currentDeck+"]","scratch2",0);
    engine.setValue("[Channel"+currentDeck+"]","scratch",0);    // Deprecated
}

/* -------- ------------------------------------------------------
    scratch.Slider
   Purpose: Uses an alpha-beta filter to make scratching with a
            slider (0..127) sound good, called each time there's a
            new slider value
   Input:   Currently-controlled Mixxx deck, value of the slider,
            revolution time of the imaginary record (typically 1.8s,
            for a 12" disc @ 33+1/3 RPM,) alpha & beta coefficients
   Output:  New value for the "scratch" control
   -------- ------------------------------------------------------ */
scratch.slider = function (currentDeck, sliderValue, revtime, alpha, beta) {
    // Skip if the track start position hasn't been set yet
    if (scratch.variables["initialTrackPos"] == -1.0) return 0;
    // If the slider start value hasn't been set yet, set it
    if (scratch.variables["initialControlValue"] == 0) {
        scratch.variables["initialControlValue"] = sliderValue;
        if (engine.getValue("[Channel"+currentDeck+"]","play") > 0) {
            scratch.variables["scratch"] = 1;
            engine.setValue("[Channel"+currentDeck+"]","scratch2", 1);
        }
//         engine.setValue("[Channel"+currentDeck+"]","scratch2_enable", 1);
//         print("Initial slider="+scratch.variables["initialControlValue"]);
    }
    return scratch.filter(currentDeck, sliderValue, revtime, alpha, beta);
}

/* -------- ------------------------------------------------------
    scratch.wheel
   Purpose: Uses an alpha-beta filter to make scratching with a
            wheel (0..127 with wrap) sound good, called each time
            there's a new wheel value
   Input:   Currently-controlled Mixxx deck, value of the wheel,
            revolution time of the imaginary record (typically 1.8s,
            for a 12" disc @ 33+1/3 RPM,) alpha & beta coefficients
   Output:  New value for the "scratch" control
   -------- ------------------------------------------------------ */
scratch.wheel = function (currentDeck, wheelValue, revtime, alpha, beta) {
    // Skip if the track start position hasn't been set yet
    if (scratch.variables["initialTrackPos"] == -1.0) return 0;
    // If the wheel start value hasn't been set yet, set it
    if (scratch.variables["initialControlValue"] == 0) {
        scratch.variables["initialControlValue"] = scratch.variables["prevControlValue"] = wheelValue;
        if (engine.getValue("[Channel"+currentDeck+"]","play") > 0) {
            scratch.variables["scratch"] = 1;
            engine.setValue("[Channel"+currentDeck+"]","scratch2", 1);
        }
//         engine.setValue("[Channel"+currentDeck+"]","scratch2_enable", 1);
//         print("Initial wheel="+scratch.variables["initialControlValue"]);
    }
        
    // Take wrap around into account
    if (wheelValue>=0 && wheelValue<10 && scratch.variables["prevControlValue"]>117 && scratch.variables["prevControlValue"]<=127) scratch.variables["wrapCount"]+=1;
    if (wheelValue>117 && wheelValue<=127 && scratch.variables["prevControlValue"]>=0 && scratch.variables["prevControlValue"]<10) scratch.variables["wrapCount"]-=1;
    
//     From radiomark: change = (new - old + 192) % 128 - 64
    
    scratch.variables["prevControlValue"]=wheelValue;
    wheelValue += scratch.variables["wrapCount"]*128;
    
    return scratch.filter(currentDeck, wheelValue, revtime, alpha, beta);
}

// The actual alpha-beta filter
scratch.filter = function (currentDeck, controlValue, revtime, alpha, beta) {
    // ------------- Thanks to Mark Hills of Xwax (http://www.xwax.co.uk) for the info for below ------------------------
    
    // ideal position = (initial_p + (y - x) / 128 * 1.8)
    var ideal_p = scratch.variables["initialTrackPos"] + (controlValue - scratch.variables["initialControlValue"]) / 128 * revtime;
    
    var currentTrackPos = engine.getValue("[Channel"+currentDeck+"]","visual_playposition") * engine.getValue("[Channel"+currentDeck+"]","duration");
    var newTime = new Date()/1000;
    var dt = newTime - scratch.variables["time"];
    scratch.variables["time"] = newTime;
//     print("dt="+dt);
    
    // predicted_p = p + dt * pitch; // Where "pitch" = 1.0 for regular speed, i.e. the "scratch" control.
    var predicted_p = currentTrackPos + dt * scratch.variables["scratch"];
    // rx = where_finger_corresponds_to - predicted_p;
    var rx = ideal_p - predicted_p;
    
    // p += rx * ALPHA;
    // scratch.variables["trackPos"] += rx * alpha;   // Don't need this result so why waste the CPU time?
    
    // v += rx * BETA / dt;
//     scratch.variables["scratch"] += rx * beta / dt;   // This doesn't work
    scratch.variables["scratch"] = rx * beta;
    
//     print("MIDI Script: Ideal position="+ideal_p+", Predicted position="+predicted_p + ", New scratch val=" + scratch.variables["scratch"]);
    
//     var newPos = scratch.variables["trackPos"]/engine.getValue("[Channel"+currentDeck+"]","duration");
//     engine.setValue("[Channel"+currentDeck+"]","visual_playposition",newPos);
//     engine.setValue("[Channel"+currentDeck+"]","scratch",scratch.variables["scratch"]);

    return scratch.variables["scratch"];
}
// ----------------- END Scratching functions --------------------

// ----------------- Object definitions --------------------------


ButtonState = {"released":0x00, "pressed":0x7F};
LedState =  {"off": 0x00, "on": 0x7F};

//Controller
function Controller () {
   this.group = "[Master]";
   this.Controls = [];
   this.Buttons = [];
};

Controller.prototype.addButton = function(buttonName, button, eventHandler) {
   if(eventHandler) {
      var executionEnvironment = this;
      function handler(value) {
         button.state = value;
         executionEnvironment[eventHandler](value);
      }
      button.handler = handler;
   }
   this.Buttons[buttonName] = button; 
}

Controller.prototype.setControlValue = function(control, value) {
   this.Controls[control].setValue(this.group, value);
}

//Button
function Button(controlId) {
   this.controlId = controlId;
   this.state = ButtonState.released;
}
Button.prototype.handleEvent = function(value) {
   this.handler(value);
};

//Control
function Control(mappedFunction, softMode) {
   this.minInput = 0;
   this.midInput = 0x3F;
   this.maxInput = 0x7F;
   this.minOutput = -1.0;
   this.midOutput = 0.0;
   this.maxOutput = 1.0;
   this.mappedFunction = mappedFunction;
   this.softMode = softMode;
   this.maxJump = 10;
}

Control.prototype.setValue = function(group, inputValue){
   var outputValue = 0;
   if(inputValue <= this.midInput){
      outputValue = this.minOutput + ((inputValue - this.minInput) / (this.midInput - this.minInput)) * (this.midOutput - this.minOutput);
   } else {
      outputValue = this.midOutput + ((inputValue - this.midInput) / (this.maxInput - this.midInput)) * (this.maxOutput - this.midOutput);
   }
   if(this.softMode){ 
      var currentValue = engine.getValue(group, this.mappedFunction);
      var currentRelative = 0.0;
      if(currentValue <= this.midOutput){
         currentRelative = this.minInput + ((currentValue - this.minOutput) / (this.midOutput - this.minOutput)) * (this.midInput - this.minInput);
      } else {
         currentRelative = this.midInput + ((currentValue - this.midOutput) / (this.maxOutput - this.midOutput)) * (this.maxInput - this.midInput);
      }
      if(inputValue > currentRelative - this.maxJump && inputValue < currentRelative + this.maxJump) {
         engine.setValue(group, this.mappedFunction, outputValue);
      }
   } else {
      engine.setValue(group, this.mappedFunction, outputValue);
   }
}

//Deck
Deck = function (deckNumber, group) {
   this.deckNumber = deckNumber;
   this.group = group;
   this.Buttons = [];
}
Deck.prototype.setControlValue = Controller.prototype.setControlValue;
Deck.prototype.addButton = Controller.prototype.addButton;

// ----------------- END Object definitions ----------------------
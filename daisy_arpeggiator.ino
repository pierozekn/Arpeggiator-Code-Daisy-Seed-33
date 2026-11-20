/// THIS PROJECT IS AN IS AN ARPEGGIATOR
#include "DaisyDuino.h"
DaisyHardware hw;

//variables
size_t num_channels;
static Oscillator osc;
float osc_bpm;
static Adsr env;
static Metro tick;
float samplerate;
volatile bool tickFlag = false;
volatile bool gate = false;
volatile int gateSamplesLeft = 0;


//buttons chord scale degree selected state
enum scaledegree {
  first,
  third,
  fourth,
  fifth,
  sixth
};

float ffirst = 60;
float tthird = 64;
float ffourth = 65;
float ffifth = 67;
float ssixth = 69;

// chord type + midi arrays
enum tonality {
  maj,
  minor,
  maj_seventh,
  min_seventh
};

const int major_chord[4] = {0, 4, 7, 12};
const int minor_chord[4] = {0, 3, 7, 12};
const int major_seventh_chord[4] = {0, 4, 7, 11};
const int minor_seventh_chord[4] ={0, 3, 7, 10};

// initial selected types
scaledegree selecteddegree = first;
tonality selectedtonality = maj;
bool descending = false;

/// buttons
const int degreePins[5] = {A0, A1, A2, A3, A4};


///// FUNCTION DECLARATIONS //////

void generateChord() {

    const int* chord;

    // Select chord array using your chord variable
    if (selectedtonality == maj) {
        chord = major_chord;
    }
    else if (selectedtonality == minor) {
        chord = minor_chord;
    }
    else if (selectedtonality == maj_seventh) {
        chord = major_seventh_chord;
    }
    else if (selectedtonality == min_seventh){
        chord = minor_seventh_chord;
    }



    // Select root note from scale degree
    float rootNote;

    switch (selecteddegree) {
        case first:
            rootNote = ffirst;
            break;

        case third:
            rootNote = tthird;
            break;

        case fourth:
            rootNote = ffourth;
            break;

        case fifth:
            rootNote = ffifth;
            break;

        case sixth:
            rootNote = ssixth;
            break;
    }

    // Select note position
    static int arpStep = 0;

    int index;

    if (descending) {
        index = 3 - arpStep;
    }
    else {
        index = arpStep;
    }

    // Calculate MIDI note
    int note = (int)rootNote + chord[index];

    // Convert MIDI note to frequency
    float frequency = 440.0f * powf(2.0f, (note - 69) / 12.0f);

    // Set oscillator frequency
    osc.SetFreq(frequency);

    // Trigger envelope
    gateSamplesLeft = (int)(samplerate * (60.0f / osc_bpm) * 0.8f);
    gate = true;

    // Move to next note
    arpStep++;

    if (arpStep >= 4) {
        arpStep = 0;
    }
}

void MyCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (tick.Process()) {
      tickFlag = true;
    }

    if (gate && --gateSamplesLeft <= 0) {
      gate = false;
    }

    float sig = osc.Process() * env.Process(gate);

    for (size_t chn = 0; chn < num_channels; chn++) {
      out[chn][i] = sig;
    }
  }
}


void readButtons() {

  for (int i = 0; i < 5; i++) {
    if (digitalRead(degreePins[i]) == LOW) {
      selecteddegree = (scaledegree)i;
    }
  }

  // A10 selects major or minor
  bool minorMode = (digitalRead(A10) == LOW);

  // A9 adds a seventh
  bool seventhMode = (digitalRead(A9) == LOW);

  if (minorMode && seventhMode) {
    selectedtonality = min_seventh;
  }
  else if (minorMode) {
    selectedtonality = min;
  }
  else if (seventhMode) {
    selectedtonality = maj_seventh;
  }
  else {
    selectedtonality = maj;
  }

  // A8 ascending/descending
  descending = (digitalRead(A8) == LOW);

  // A7 tempo
  osc_bpm = 60.0f + (analogRead(A7) / 1023.0f) * 180.0f;
  tick.SetFreq(osc_bpm / 60.0f);
}

void setup() {
  //initializing daisy at 48kHz
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  num_channels = hw.num_channels;
  samplerate = DAISY.get_samplerate();

  
  env.Init(samplerate);
  osc.Init(samplerate);

  // Set up metro to pulse every second
  tick.Init(1.0f, samplerate);

  // Set envelope parameters
  env.SetTime(ADSR_SEG_ATTACK, .1);
  env.SetTime(ADSR_SEG_DECAY, .1);
  env.SetTime(ADSR_SEG_RELEASE, .01);

  env.SetSustainLevel(.25);

  // Set parameters for oscillator
  osc.SetWaveform(WAVE_POLYBLEP_TRI);
  osc.SetFreq(440);
  osc.SetAmp(2.0);

  for (int i = 0; i < 5; i++) {
  pinMode(degreePins[i], INPUT_PULLUP);
}

pinMode(A8, INPUT_PULLUP);
pinMode(A9, INPUT_PULLUP);
pinMode(A10, INPUT_PULLUP);

  DAISY.begin(MyCallback);

}

void loop() {
  readButtons();
   if (tickFlag) {
    tickFlag = false;
    generateChord();
  }

}

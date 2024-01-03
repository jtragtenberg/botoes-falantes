#include "MIDIUSB.h"
#include <Bounce2.h>

const byte QTDE_BOTOES = 3;

const int MIDI_CHANNEL = 10;

byte pinosBotoes[QTDE_BOTOES] = {2, 3, 4};
byte pinoSensor = A3;
byte pinoBotaoModo = 15;
byte pinoLedBotaoModo = A1;

byte pagina = 0;
const byte QTDE_PAGINAS = 3;


byte notaMidi[QTDE_PAGINAS][QTDE_BOTOES] = {
  {60, 61, 62},
  {63, 64, 65},
  {66, 67, 68}
};

byte ccMidiSensor = 11;

int leituraAnalogicaAtual;
int ultimaLeituraAnalogica;
float leituraAnalogicaFiltrada;

Bounce botao1 = Bounce();
Bounce botao2 = Bounce();
Bounce botao3 = Bounce();
Bounce botaoModo = Bounce();

Bounce* botoes[QTDE_BOTOES] = {&botao1, &botao2, &botao3};

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < QTDE_BOTOES; i++) {
    pinMode(pinosBotoes[i], INPUT_PULLUP);
    botoes[i]->attach(pinosBotoes[i]);
    botoes[i]->interval(10);
  }
  botaoModo.attach(pinoBotaoModo, INPUT_PULLUP);
  botaoModo.interval(10);
  pinMode(pinoLedBotaoModo, OUTPUT);
}

void loop() {
  //  controlChange(MIDI_CHANNEL - 1, ccMidiSensor, analogRead(A3)/8);
  leituraAnalogicaAtual = analogRead(pinoSensor);
  leituraAnalogicaFiltrada = filtrar(leituraAnalogicaAtual, leituraAnalogicaFiltrada, 0.9);
  int leituraAnalogicaMapeada = map(leituraAnalogicaFiltrada, 30, 1010, 0, 127);
  if (leituraAnalogicaMapeada != ultimaLeituraAnalogica) {
    //      MIDI.sendControlChange(ccMIDI[i], leituraAnalogicaMapeada, MidiChan);
    controlChange(MIDI_CHANNEL - 1, ccMidiSensor, leituraAnalogicaMapeada);
    MidiUSB.flush();
  }
  ultimaLeituraAnalogica = leituraAnalogicaMapeada;

  for (int i = 0; i < QTDE_BOTOES; i++) {
    botoes[i]->update();
    int index = i + 1;
    if (botoes[i]->fell()) {
      //      Serial.print("Apertou: botão ");
      //      Serial.println(i);
      //      MIDI.sendNoteOn(notasMIDI[i], 127, MidiChan);
      noteOn(MIDI_CHANNEL - 1, notaMidi[pagina][i], 127);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();
    }
    if (botoes[i]->rose()) {
      //      Serial.print("Levantou: botão ");
      //      Serial.println(i);
      //      MIDI.sendNoteOff(notasMIDI[i], 0, MidiChan);
      noteOff(MIDI_CHANNEL - 1, notaMidi[pagina][i], 0);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();
    }
  }

  botaoModo.update();
  if (botaoModo.fell()) {
    digitalWrite(pinoLedBotaoModo, HIGH);
    pagina++;
    if (pagina >= QTDE_PAGINAS) {
      pagina = 0;
    }
//    Serial.println("APERTOU MODO");
//    Serial.println(pagina);
  }
  if (botaoModo.rose()) {
    digitalWrite(pinoLedBotaoModo, LOW);
//    Serial.println("SOLTOU MODO");
  }

}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

int filtrar(int valEntrada, int valFiltrado, float qtdeDeFiltragem) {
  return ((1 - qtdeDeFiltragem) * valEntrada) + (qtdeDeFiltragem * valFiltrado);
}

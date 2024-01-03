#include "MIDIUSB.h"
#include <Bounce2.h>

const byte QTDE_BOTOES = 3;
const int MIDI_CHANNEL = 10;
const byte QTDE_PAGINAS = 3;
const byte TEMPO_LEITURA_PIEZO = 10;

const byte pinosBotoes[QTDE_BOTOES] = {2, 3, 4};
const byte pinoSensor = A3;
const byte pinoBotaoModo = 15;
const byte pinoLedBotaoModo = A1;
const byte pinoPiezo = A0;

int velocityPiezo = 127;
int velocityMinimo = 60;
int sensibilidadePiezo = 400;

byte pagina = 0;

unsigned long millisAnteriorBlink = 0;
const long intervalo = 13;  // Intervalo de tempo em milissegundos (1 segundo)


byte notaMidi[QTDE_PAGINAS][QTDE_BOTOES] = {
  {60, 61, 62},
  {63, 64, 65},
  {66, 67, 68}
};

//byte ccMidiSensor = 11;

int leituraAnalogicaAtual;
int ultimaLeituraAnalogica;
float leituraAnalogicaFiltrada;

unsigned long tempo = 0;
unsigned long tempoAnterior = 0;
int picoPiezo = 0;


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
    botoes[i]->interval(3);
  }
  botaoModo.attach(pinoBotaoModo, INPUT_PULLUP);
  botaoModo.interval(10);
  pinMode(pinoLedBotaoModo, OUTPUT);
}

void loop() {
  //  controlChange(MIDI_CHANNEL - 1, ccMidiSensor, analogRead(A3)/8);
  leituraAnalogicaAtual = analogRead(pinoSensor);
  leituraAnalogicaFiltrada = filtrar(leituraAnalogicaAtual, leituraAnalogicaFiltrada, 0.9);
  int leituraAnalogicaMapeada = map(leituraAnalogicaFiltrada, 760, 100, 8192, 16384);
  if (leituraAnalogicaMapeada < 8192) {
    leituraAnalogicaMapeada = 8192;
  }
  if (leituraAnalogicaMapeada > 16384) {
    leituraAnalogicaMapeada = 16384;
  }
  if (leituraAnalogicaMapeada != ultimaLeituraAnalogica) {
    //      MIDI.sendControlChange(ccMIDI[i], leituraAnalogicaMapeada, MidiChan);
    //    controlChange(MIDI_CHANNEL - 1, ccMidiSensor, leituraAnalogicaMapeada);
    //    Serial.println(leituraAnalogicaFiltrada);
    pitchBend(MIDI_CHANNEL - 1, leituraAnalogicaMapeada);
    MidiUSB.flush();

  }
  ultimaLeituraAnalogica = leituraAnalogicaMapeada;

  for (int i = 0; i < QTDE_BOTOES; i++) {
    botoes[i]->update();
    int index = i + 1;
    if (botoes[i]->fell()) {
      int leituraPiezo = analogRead(pinoPiezo);
      //      picoPiezo = leituraPiezo;
      picoPiezo = 0;
      tempo = millis();
      tempoAnterior = tempo;
      while (tempo - tempoAnterior <= TEMPO_LEITURA_PIEZO) {
        //        Serial.println(tempo - tempoAnterior);
        leituraPiezo = analogRead(pinoPiezo);
        if (leituraPiezo > picoPiezo) {
          picoPiezo = leituraPiezo;
          //          Serial.println(picoPiezo);
        }
        tempo = millis();
      }
      //      Serial.println();
      //      Serial.print("Imprimiu um pico massa: ");
      //      Serial.println(picoPiezo);
      velocityPiezo = map(picoPiezo, 0, sensibilidadePiezo, velocityMinimo, 127);
      if (velocityPiezo > 127) {
        velocityPiezo = 127;
      } else if (velocityPiezo < velocityMinimo) {
        velocityPiezo = velocityMinimo;
      }


      //      Serial.print("Apertou: botão ");
      //      Serial.println(i);
      //      MIDI.sendNoteOn(notasMIDI[i], 127, MidiChan);
      noteOn(MIDI_CHANNEL - 1, notaMidi[pagina][i], velocityPiezo);   // canal_midi, nota_midi, velocity_midi
      //      noteOn(MIDI_CHANNEL - 1, notaMidi[pagina][i], 100);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();
    }
    if (botoes[i]->rose()) {
      //            Serial.println("Levantou o botão ");
      //      Serial.println(i);
      //      MIDI.sendNoteOff(notasMIDI[i], 0, MidiChan);
      noteOff(MIDI_CHANNEL - 1, notaMidi[pagina][i], 0);   // canal_midi, nota_midi, velocity_midi
      MidiUSB.flush();
    }
  }

  if (pagina == 0) {
    digitalWrite(pinoLedBotaoModo, LOW);
  }

  if (pagina == 1) {
    unsigned long tempoAtualBlink = millis();

    if (tempoAtualBlink - millisAnteriorBlink >= intervalo) {
      millisAnteriorBlink = tempoAtualBlink;

      // Inverte o estado do LED (liga/desliga)
      if (digitalRead(pinoLedBotaoModo) == LOW) {
        digitalWrite(pinoLedBotaoModo, HIGH);
      } else {
        digitalWrite(pinoLedBotaoModo, LOW);
      }
    }
  }

  if (pagina == 2) {
    digitalWrite(pinoLedBotaoModo, HIGH);
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

void pitchBend(byte channel, int value) {
  // Split the 14-bit pitch bend value into two 7-bit values
  byte lsb = value & 0x7F;
  byte msb = (value >> 7) & 0x7F;

  midiEventPacket_t event = {0x0E, 0xE0 | channel, lsb, msb};
  MidiUSB.sendMIDI(event);
}


int filtrar(int valEntrada, int valFiltrado, float qtdeDeFiltragem) {
  return ((1 - qtdeDeFiltragem) * valEntrada) + (qtdeDeFiltragem * valFiltrado);
}

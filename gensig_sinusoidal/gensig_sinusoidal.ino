#include <avr/io.h>
#include <avr/interrupt.h>

int i = 0; // Index untuk mengakses nilai sinus
float pi = 3.14159;
float factor = pi / 180;
float targetFrequency = 5; // Target frekuensi 0,05 Hz (20 detik per siklus)
volatile int sineValue; // Variabel untuk nilai sinus yang akan dikeluarkan
unsigned long startTime, stopTime;
volatile bool cycleComplete = false;

// Variables for amplitude calculation
volatile int minValue = 255; // Initialize to max possible 8-bit value
volatile int maxValue = 0;   // Initialize to min possible 8-bit value

void setup() {
  Serial.begin(9600);
  Serial.println("Gelombang Sinus dengan Timer Hardware");
  DDRD = 0xFF; // Set PORTD sebagai output

  // Setup Timer1
  cli(); // Matikan interrupt global saat konfigurasi
  TCCR1A = 0; // Normal mode
  TCCR1B = 0;
  TCNT1 = 0; // Set nilai awal Timer1

  // Hitung OCR1A berdasarkan frekuensi target
  // Interval = (1 / targetFrequency) / jumlah_titik
  float intervalTime = (1.0 / targetFrequency) / 50.0; // 50 titik per siklus (Arbitrary)
  // Menentukan nilai OCR agar timer berhitung sampai N (Timer hitung sampai N+1 sehingga rumus harus -1)
  OCR1A = (int)(intervalTime * (16000000 / 1024)) - 1;

  TCCR1B |= (1 << WGM12); // Mode CTC (Clear Timer on Compare Match)
  TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler 1024
  TIMSK1 |= (1 << OCIE1A); // Aktifkan interrupt Timer1 pada compare match
  sei(); // Aktifkan interrupt global

  startTime = millis(); // Mulai waktu awal siklus pertama
}

ISR(TIMER1_COMPA_vect) {
  // Hitung nilai sinus dan keluarkan ke PORTD
  float y1 = sin(factor * i * (360.0 / 50)) + 1.0; // Nilai sinus dengan offset
  sineValue = round(127 * y1); // Skala ke 8-bit (0-255)
  PORTD = sineValue; // Output ke PORTD untuk DAC R-2R

  // Update min and max values
  if (sineValue < minValue) minValue = sineValue;
  if (sineValue > maxValue) maxValue = sineValue;

  i++; // Naikkan indeks
  if (i >= 50) { // Reset indeks setelah satu siklus selesai
    i = 0;
    cycleComplete = true;
    stopTime = millis(); // Catat waktu akhir siklus
  }
}

void loop() {
  Serial.println(sineValue);
  if (cycleComplete) {
    // Calculate frequency
    unsigned long duration = stopTime - startTime;
    float freq = 1000.0 / duration; // Menghitung frekuensi dalam Hz
    
    // Calculate amplitude as half of the peak-to-peak
    int peakToPeak = maxValue - minValue;
    float amplitude = peakToPeak / 2.0;

    // Print frequency, amplitude, and sine value
    // Serial.print("Frekuensi terbaca: ");
    // Serial.print(freq);
    // Serial.print(" Hz, Amplitudo: ");
    // Serial.print(amplitude);
    // Serial.println(" (8-bit units)");

    // Reset variables for the next cycle
    cycleComplete = false;
    startTime = millis(); // Set waktu mulai siklus baru
    minValue = 255; // Reset min value
    maxValue = 0;   // Reset max value
  }
}

#include <Arduino.h>
#include <driver/i2s.h>

// --- Konfigurasi Pin ---
const int micPin   = 3; // Pin ADC untuk MAX9814 (Mic)
const int I2S_BCLK = 5; // Pin Clock I2S (Ampli)
const int I2S_LRC  = 4; // Pin Word Select (Ampli)
const int I2S_DOUT = 6; // Pin Data Out (Ampli)

// --- Konfigurasi Audio Processing ---
const int DC_BIAS = 1550;       // Titik tengah sensor (kalibrasi jika dengung)
const int MULTIPLIER = 4;       // Penguatan volume (Jangan terlalu besar dulu!)
const int NOISE_GATE_THRESHOLD = 300; // Batas minimal suara agar ampli menyala

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000, // Kecepatan standar suara manusia
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 64,
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE // Tetap gunakan native driver untuk hindari bug
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Memulai Real-Time Audio (Mic -> I2S Ampli)...");

  setupI2S();
  Serial.println("Sistem aktif! Coba bicara ke mikrofon.");
}

void loop() {
  // 1. Baca data raw dari Mikrofon (sekitar 9-10 microsecond)
  int adc_val = analogRead(micPin);
  
  // 2. Olah sinyal: Hapus tegangan bias dan naikkan volume
  int16_t pcm_val = (adc_val - DC_BIAS) * MULTIPLIER; 

  // 3. Terapkan Noise Gate (Mencegah Feedback jika mic & speaker dekat)
  // abs() mengubah nilai negatif jadi positif untuk mengecek seberapa besar getarannya
  if (abs(pcm_val) < NOISE_GATE_THRESHOLD) {
    pcm_val = 0; // Jika di bawah ambang batas, paksa hening
  }

  // 4. Kirim ke Ampli via I2S
  // Fungsi ini otomatis menahan (block) perputaran loop agar sesuai kecepatan 16000Hz
  size_t bytes_written;
  i2s_write(I2S_NUM_0, &pcm_val, sizeof(pcm_val), &bytes_written, portMAX_DELAY);
}
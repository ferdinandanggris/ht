#include <Arduino.h>
#include <driver/i2s.h>

const int micPin   = 3; 
const int I2S_BCLK = 5; 
const int I2S_LRC  = 4; 
const int I2S_DOUT = 6; 

const int DC_BIAS = 1550;       
const int MULTIPLIER = 5;       

const int BUFFER_SIZE = 64;     
int16_t tx_buffer[BUFFER_SIZE]; 

// Konstanta waktu untuk 16000 Hz (1.000.000 mikrodetik dibagi 16.000)
const unsigned long SAMPLE_INTERVAL_US = 62; 

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate          = 16000, 
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = BUFFER_SIZE, 
    .use_apll             = false,
    .tx_desc_auto_clear   = true,
    .fixed_mclk           = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Memulai Audio Bridge (High Precision + Anti-Clip)...");
  setupI2S();
}

void loop() {
  unsigned long nextSampleTime = micros();

  // 1. Isi buffer dengan penjadwalan waktu yang sangat presisi
  for (int i = 0; i < BUFFER_SIZE; i++) {
    
    // Tahan loop di sini sampai waktu persis menyentuh interval 62 mikrodetik
    while (micros() < nextSampleTime) {
      // Tunggu... (tight loop)
    }
    nextSampleTime += SAMPLE_INTERVAL_US; // Jadwalkan untuk sampel berikutnya

    // Baca sensor
    int adc_val = analogRead(micPin);
    
    // Gunakan long integer sementara agar nilai tidak bocor saat dikalikan
    long pcm_calc = (adc_val - DC_BIAS) * MULTIPLIER;
    
    // --- ANTI CLIPPING PROTECTION ---
    // Paksa nilai agar tetap di dalam batas aman 16-bit
    if (pcm_calc > 32767) pcm_calc = 32767;
    if (pcm_calc < -32768) pcm_calc = -32768;

    // Masukkan ke ember
    tx_buffer[i] = (int16_t)pcm_calc;
  }

  // 2. Tembak data ke Ampli
  size_t bytes_written;
  i2s_write(I2S_NUM_0, tx_buffer, sizeof(tx_buffer), &bytes_written, portMAX_DELAY);
}
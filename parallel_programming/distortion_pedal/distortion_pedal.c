#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

// --- CONFIGURATION ---
#define NUM_THREADS 12
// 44100 samples is exactly 1 second of audio in a Mono track, 
// or 0.5 seconds in a Stereo track (since Left/Right alternate).
#define DELAY_SAMPLES 44100 
#define MASTER_VOLUME 0.5f

// --- THREAD DATA STRUCTURE ---
typedef struct {
    int16_t* source_audio;
    int16_t* dest_audio;
    size_t start_index;
    size_t end_index;
} ThreadData;

// --- TIMER FUNCTION ---
double get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + (t.tv_usec / 1000000.0);
}

// --- SEQUENTIAL IMPLEMENTATION ---
void process_sequential(int16_t* source, int16_t* dest, size_t num_samples) {
    for (size_t i = 0; i < num_samples; i++) {
        float sample = source[i]; // Start with the original sound

        // Add 3 fading echoes (looking backwards in time)
        if (i >= DELAY_SAMPLES)     sample += source[i - DELAY_SAMPLES] * 0.6f;
        if (i >= DELAY_SAMPLES * 2) sample += source[i - (DELAY_SAMPLES * 2)] * 0.3f;
        if (i >= DELAY_SAMPLES * 3) sample += source[i - (DELAY_SAMPLES * 3)] * 0.15f;

        sample *= MASTER_VOLUME;

        // Prevent clipping when the volume gets too loud from adding echoes
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;

        dest[i] = (int16_t)sample;
    }
}

// --- PARALLEL THREAD FUNCTION ---
void* thread_process(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (size_t i = data->start_index; i < data->end_index; i++) {
        float sample = data->source_audio[i];

        // Threads safely read backwards in time because the source array is strictly Read-Only!
        if (i >= DELAY_SAMPLES)     sample += data->source_audio[i - DELAY_SAMPLES] * 0.6f;
        if (i >= DELAY_SAMPLES * 2) sample += data->source_audio[i - (DELAY_SAMPLES * 2)] * 0.3f;
        if (i >= DELAY_SAMPLES * 3) sample += data->source_audio[i - (DELAY_SAMPLES * 3)] * 0.15f;

        sample *= MASTER_VOLUME;

        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;

        data->dest_audio[i] = (int16_t)sample;
    }
    return NULL;
}

// --- PARALLEL IMPLEMENTATION ---
void process_parallel(int16_t* source, int16_t* dest, size_t num_samples) {
    pthread_t threads[NUM_THREADS];
    ThreadData t_data[NUM_THREADS];
    size_t chunk_size = num_samples / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        t_data[i].source_audio = source;
        t_data[i].dest_audio = dest;
        t_data[i].start_index = i * chunk_size;
        
        if (i == NUM_THREADS - 1) {
            t_data[i].end_index = num_samples;
        } else {
            t_data[i].end_index = (i + 1) * chunk_size;
        }
        pthread_create(&threads[i], NULL, thread_process, &t_data[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

// --- MAIN ENIGNE ---
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input.wav> <output.wav>\n", argv[0]);
        return 1;
    }

    FILE* file_in = fopen(argv[1], "rb");
    if (!file_in) {
        printf("Error: Could not open %s\n", argv[1]);
        return 1;
    }

    // 1. VERIFY IT IS A WAV FILE
    char riff_header[12];
    fread(riff_header, 1, 12, file_in);
    if (strncmp(riff_header, "RIFF", 4) != 0 || strncmp(riff_header + 8, "WAVE", 4) != 0) {
        printf("Error: Not a valid WAV file.\n");
        fclose(file_in);
        return 1;
    }

    // 2. SMART CHUNK SCANNER: Find where the audio data starts
    char chunk_id[4];
    uint32_t chunk_size;
    long data_offset = 0;

    while (fread(chunk_id, 1, 4, file_in) == 4) {
        fread(&chunk_size, 4, 1, file_in);
        if (strncmp(chunk_id, "data", 4) == 0) {
            data_offset = ftell(file_in);
            break;
        }
        fseek(file_in, chunk_size, SEEK_CUR);
    }

    if (data_offset == 0) {
        printf("Error: Could not find audio data.\n");
        fclose(file_in);
        return 1;
    }

    // 3. LOAD THE DATA
    uint32_t audio_data_size = chunk_size;
    size_t num_samples = audio_data_size / 2; // 2 bytes per 16-bit sample
    
    printf("WAV File Parsed Successfully!\n");
    printf("Total Samples: %zu (%.2f Megabytes)\n\n", num_samples, (float)audio_data_size / 1024 / 1024);

    // Save header to write back later
    rewind(file_in);
    uint8_t* header_bytes = (uint8_t*)malloc(data_offset);
    fread(header_bytes, 1, data_offset, file_in);

    // Memory Allocation
    int16_t* original_audio = (int16_t*)malloc(audio_data_size);
    int16_t* working_audio = (int16_t*)malloc(audio_data_size);
    
    // Read the file into original_audio
    fread(original_audio, 2, num_samples, file_in);
    fclose(file_in);

    // --- BENCHMARK 1: SEQUENTIAL ---
    double seq_start = get_time();
    process_sequential(original_audio, working_audio, num_samples);
    double seq_time = get_time() - seq_start;
    printf("[Sequential] Processing Time: %f seconds\n", seq_time);

    // --- BENCHMARK 2: PARALLEL ---
    double par_start = get_time();
    process_parallel(original_audio, working_audio, num_samples);
    double par_time = get_time() - par_start;
    printf("[Parallel %d Cores] Processing Time: %f seconds\n", NUM_THREADS, par_time);

    // --- CALCULATE SPEEDUP & EFFICIENCY ---
    double speedup = seq_time / par_time;
    double efficiency = speedup / NUM_THREADS; // Speedup divided by total cores

    printf("\n========================================\n");
    printf(">>> SPEEDUP FACTOR: %.2fx\n", speedup);
    printf(">>> CPU EFFICIENCY: %.2f%%\n", efficiency * 100.0);
    printf("========================================\n");
    
    if (speedup < 1.0) {
        printf("\nNote: Parallel was SLOWER! The audio file is likely too small, meaning thread creation overhead took longer than the actual processing.\n");
    } else if (efficiency < 0.7) {
        printf("\nNote: Efficiency is dropping. You are hitting the limits of Amdahl's Law, or memory bandwidth is becoming a bottleneck.\n");
    }

    // 4. WRITE THE OUTPUT FILE
    FILE* file_out = fopen(argv[2], "wb");
    if (file_out) {
        fwrite(header_bytes, 1, data_offset, file_out);
        fwrite(working_audio, 2, num_samples, file_out);
        fclose(file_out);
        printf("\nSaved Echo audio to: %s\n", argv[2]);
    }

    free(header_bytes);
    free(original_audio);
    free(working_audio);
    return 0;
}
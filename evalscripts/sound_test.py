from pydub import AudioSegment

AUDIO_FILE = "input.flac"

def play_sound(delay, chunk_length):
    speech = AudioSegment.from_file(AUDIO_FILE)
    chunks = [speech[i:i + chunk_length] for i in range(0, len(speech), chunk_length)]
    silence = AudioSegment.silent(duration=delay)    
    output = AudioSegment.empty()
    for chunk in chunks:
        output += chunk + silence
    output.export(f"output-{delay}.wav", format="wav")

if __name__ == '__main__':
    snippet_length = 100
    delays = [0.02,0.05,0.1,0.2]
    for delay in delays:
        play_sound(delay, snippet_length)




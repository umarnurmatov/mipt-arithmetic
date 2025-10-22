from subprocess import Popen, PIPE

def video_to_asm(video_path, width=640, height=480):
    command = [
        'ffmpeg',
        '-i', video_path,
        '-f', 'image2pipe',
        '-pix_fmt', 'rgb24',
        '-vcodec', 'rawvideo',
        '-'
    ]
    
    pipe = Popen(command, stdout=PIPE, stderr=PIPE)
    bytes_per_frame = width * height * 3
    
    while True:
        raw_data = pipe.stdout.read(bytes_per_frame)

        print(raw_data)
            
        # Convert to 2D array of pixels
        with open("prog-video.asm", "w") as file:
            for y in range(height):
                for x in range(width):
                    idx = (y * width + x) * 3
                    r = raw_data[idx]
                    g = raw_data[idx + 1]
                    b = raw_data[idx + 2]
    
    pipe.terminate()

if __name__ == "__main__":
    video_to_asm('video.mp4', 480, 360)

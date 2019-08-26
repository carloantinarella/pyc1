from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.widget import Widget
from kivy.graphics.texture import Texture
from kivy.clock import Clock
from kivy.properties import ObjectProperty

from array import array
import socket
import os, os.path
import struct
import mmap
import datetime

SRV_SOCK_PATH = "/tmp/ud_rnd"
CLN_SOCK_PATH = "/tmp/ud_cln_rnd"
SHM_FILE = "/dev/shm/ud_rnd_shmem"
SIZE_X = 32
SIZE_Y = 32
PIXEL_SIZE = 16
BUF_SIZE = (SIZE_X * SIZE_Y * 3)

class ColorGrid():

    def __init__(self):
        self.size_x = SIZE_X
        self.size_y = SIZE_Y
        self.pixel_size = 16
        self.texture = Texture.create(size=(self.size_x * self.pixel_size,
                                            self.size_y * self.pixel_size))
        # open shared memory
        self.fd = os.open(SHM_FILE, os.O_RDONLY)
        self.grid_file = mmap.mmap(self.fd, BUF_SIZE, mmap.MAP_SHARED, mmap.PROT_READ)

    def get_texture(self):
        ''' Read the texture from shared memory and scale up pixels using pixel replication algorithm '''
        size = self.size_x * self.size_y # number of big pixels
        byte_size = size * 3
        big_byte_size = byte_size * self.pixel_size * self.pixel_size
        big_buf = [255 for x in range(big_byte_size)]

        # fill the buffer with file data
        buf = self.grid_file[:]

        # Pixel replication
        m = 0
        for i in range(self.size_y):
            for q in range(self.pixel_size):
                for j in range(0, self.size_x * 3, 3):
                    for k in range(self.pixel_size):
                        for c in range(3): # bytes per pixel
                            big_buf[m] = buf[i*self.size_x*3 + j + c]
                            m += 1

        self.arr = array('B', big_buf)
        self.texture.blit_buffer(self.arr, colorfmt='rgb', bufferfmt='ubyte')
        return self.texture


class MainLayout(BoxLayout):
    main_layout = ObjectProperty()

    def start_acquisition(self):
        ''' First texture acquisition and launching of interval acquisition '''
        self.texture = Texture.create(size=(SIZE_X * PIXEL_SIZE,
                                            SIZE_Y * PIXEL_SIZE))
        self.colorgrid = ColorGrid()
        self.acquire(1)
        self.acquisition_event = Clock.schedule_interval(self.acquire, 0.5)


    def acquire(self, dt):
        ''' Acquire new texture, apply it to displayed image, and force canvas update '''
        self.texture = self.colorgrid.get_texture()
        self.main_layout.ids.main_image.texture = self.texture
        self.main_layout.ids.main_image.size = self.texture.size
        self.canvas.ask_update()

    def start_udp_client(self):
        ''' Connect to the UDP server created by the C process '''
        if os.path.exists(CLN_SOCK_PATH):
            os.remove(CLN_SOCK_PATH)
        if os.path.exists(SRV_SOCK_PATH):
            self.client = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
            self.client.bind(CLN_SOCK_PATH)
            self.client.connect(SRV_SOCK_PATH)
            print("Connected to socket")

    def send_nsamples(self):
        ''' Send UDP message to the C program containing the number of the averaged samples required '''
        snd = struct.pack('i', int(self.main_layout.ids.sl1.value))
        self.client.send(snd)

    def save_image(self):
        ''' Export on-display textured image to external png file '''
        now = datetime.datetime.now()
        filename = "{:04d}{:02d}{:02d}_{:02d}{:02d}{:02d}.png".format(now.year, now.month, now.day, now.hour, now.minute, now.second)
        self.main_layout.ids.main_image.export_to_png(filename)
        print("Saving image to " + filename)

class PycApp(App):
    ''' Main application '''

    def build(self):
        self.ml = MainLayout()
        self.ml.start_acquisition()
        self.ml.start_udp_client()
        return self.ml

    def on_stop(self):
        ''' By sending 999 to the C process we order it to exit '''
        print("Exiting python application...")
        snd = struct.pack('i', int(999))
        self.ml.client.send(snd)

if __name__ == '__main__':
    PycApp().run()

import threading
import serial
from serial.tools import list_ports
import time
from collections import deque

from dash import Dash, dcc, html, Input, Output, dash_table
import plotly



class DataStorage:
    def __init__(self, max_size=10000):
        self.data = deque(maxlen=max_size)
        self.max_size = max_size

    def add_data(self, packet: str, time_of_packet: float):
        self.data.append((packet, time_of_packet))

    def get_data(self):
        return list(self.data)

def read_from_serial(arduino : serial.Serial, buffer : DataStorage, lock : threading.Lock, new_data_event : threading.Event):
    while True:
        # Read byte from serial if it exists
        try:
            bytes_to_read = arduino.in_waiting
            if bytes_to_read > 0:
                raw = arduino.read(bytes_to_read)
                current_time = time.time_ns() / 1000
                with lock:
                    buffer.add_data(raw.hex(), current_time)
                new_data_event.set()

        except Exception as e:
            print(f"Serial read error: {e}")
            break
            
def process_data(buffer : DataStorage, lock : threading.Lock, new_data_event : threading.Event, sorted_buffer : DataStorage):
    while True:
        new_data_event.wait()  # Wait for new data to be read
        new_data_event.clear()

        with lock:
            while len(buffer.data) > 0:
                packet, time_of_packet = buffer.data.popleft()
                sorted_buffer.add_data(packet, time_of_packet)



def listPorts():
    port = list(list_ports.comports())
    portchoice = []
    for p in port:
        portchoice.append(p.device)
    return portchoice


def ChoosePort() -> str:
    print("\n\n\n")
    print("Please select a port from the list below\n")
    port = list(list_ports.comports())
    for i, p in enumerate(port):
        print(f"{i+1}. {p.device}")

    choosenPort = int(input("\nPort: "))

    return port[choosenPort-1].device


app = Dash(__name__)
app.layout = html.Div([
    dash_table.DataTable(
        id='data-table',
        columns=[{'name': 'Time', 'id': 'time', 'type': 'numeric', 'width': '10%'}, {'name': 'Data', 'id': 'data'}]

    ),
    dcc.Interval(
        id='graph-update',
        interval=1000, # in milliseconds
        n_intervals=0
    )
])

def GUI():
    app.run_server(debug=False, port=8050)







def main():
    chosen_port = ChoosePort()
    arduino = serial.Serial(chosen_port, 115200)
    buffer = DataStorage()
    sorted_buffer = DataStorage()
    lock = threading.Lock()
    new_data_event = threading.Event()


    try:
        # Create a new thread for the serial read and processing and GUI
        reader_thread = threading.Thread(target=read_from_serial, args=(arduino, buffer, lock, new_data_event))
        processor_thread = threading.Thread(target=process_data, args=(buffer, lock, new_data_event, sorted_buffer))
        GUI_thread = threading.Thread(target=GUI)

        reader_thread.start()
        processor_thread.start()
        GUI_thread.start()

        reader_thread.join()
        processor_thread.join()
        GUI_thread.join()

    except KeyboardInterrupt:
        pass

    finally:
        arduino.close()

    return sorted_buffer



main()
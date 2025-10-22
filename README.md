
# C++ & Python Hybrid Ad Blocker

![Adblocker GUI](adblockss.png)

A high-performance, DNS-based ad blocker built with a C++ backend and a modern Python (PyQt) frontend.

This project demonstrates a hybrid architecture:
* **C++ Backend:** A lightweight, multi-platform DNS server that binds to port 53. It handles all network-heavy tasks, parses DNS queries, and filters domains against a blocklist with high efficiency.
* **Python (PyQt) Frontend:** A sleek, user-friendly GUI that allows the user to easily start and stop the ad-blocking service. The Python script manages the C++ executable as a subprocess.

## How It Works

1.  The Python GUI (`app.py`) is launched (with `sudo`/Administrator rights).
2.  When the user clicks "OPEN", the GUI launches the compiled C++ executable (`adblocker`) as a background process.
3.  The C++ backend process:
    * Binds to port 53 to act as the system's DNS server.
    * Loads all domains from `backend/hosts.txt` into an `std::unordered_set` for $O(1)$ average lookups.
    * For every incoming DNS query:
        * **If the domain is in the blocklist:** It sends back a fake `0.0.0.0` IP address, preventing the ad from loading.
        * **If the domain is not in the blocklist:** It forwards the query to a real DNS server (e.g., `8.8.8.8`) and returns the genuine answer to the user.
4.  When the user clicks "CLOSE", the Python GUI terminates the C++ backend process, stopping the service.

## Tech Stack

* **Backend:** C++ (Socket Programming, UDP, `std::unordered_set`)
* **Frontend:** Python 3
* **GUI:** PyQt5 (with CSS/QSS for styling)
* **Communication:** `subprocess` management
* **Platform:** Multi-platform (Tested on macOS and Windows)

## How to Run

You must run this application with administrator/root privileges to allow the C++ backend to bind to the privileged port 53.

**1. Compile the C++ Backend:**
Navigate to the `backend/` directory and compile the C++ code:

   ```bash
   # On macOS/Linux
   g++ main.cpp -o adblocker
   
   # On Windows (with g++)
   g++ main.cpp -o adblocker.exe -lws2_32
````

**2. Install Python Dependencies:**
Make sure you have `PyQt5` installed:

```bash
pip install PyQt5
```

**3. Run the Application:**
From the project's root directory, run the Python app with `sudo`:

```bash
# On macOS/Linux
sudo python3 app.py

# On Windows
# (Run your terminal as Administrator and then run)
python app.py
```

**4. Set Your System DNS:**
To make it work, you must set your system's DNS server to `127.0.0.1`.

## Acknowledgements

This project was built by combining and expanding upon the concepts from these excellent repositories:

  * [StefanPenchev05/DNS-Server-Cpp](https://github.com/StefanPenchev05/DNS-Server-Cpp): Provided the foundational skeleton for a C++ DNS server.
  * [Beta-7/Adblockerc-](https://github.com/Beta-7/Adblockerc-): Inspired the basic ad-blocking logic via host filtering.


```
```

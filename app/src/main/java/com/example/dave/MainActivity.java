package com.example.dave;

import static android.content.ContentValues.TAG;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private BluetoothAdapter bluetoothAdapter;
    private String deviceAddress;
    private String ARDUINO_DEVICE_NAME = "HC-06";

    public static Handler handler;
    public static BluetoothSocket mmSocket;
    public static ConnectedThread connectedThread;
    public static CreateConnectThread createConnectThread;

    private final static int CONNECTING_STATUS = 1; // used in bluetooth handler to identify message status
    private final static int CONNECTING_STATUS_SUCCESS = 1;
    private final static int CONNECTING_STATUS_FAILURE = -1;
    private final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final FloatingActionButton bluethoothConnectButton = findViewById(R.id.bluethoothConnectButton);
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        final TextView connectedText = findViewById(R.id.connectedText);
        final ImageButton forwardButton = findViewById(R.id.forwardButton);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        ActivityResultLauncher<Intent> enableBluetoothActivityResultLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                new ActivityResultCallback<ActivityResult>() {
                    @Override
                    public void onActivityResult(ActivityResult result) {
                        if (result.getResultCode() == Activity.RESULT_OK) {
                            MainActivity.this.notify("Status", "Bluetooth enabled");
                            connectToArduino();
                        }
                        else {
                            MainActivity.this.notify("Status", "Bluetooth enable failed");
                        }
                    }
                });

        bluethoothConnectButton.setOnClickListener(new View.OnClickListener() {
            @SuppressLint("MissingPermission")
            @Override
            public void onClick(View view) {
                // Show progress
                progressBar.setVisibility(View.VISIBLE);
                bluethoothConnectButton.setVisibility(View.GONE);
                bluethoothConnectButton.setEnabled(false);

                if (!bluetoothAdapter.isEnabled()) {
                    Intent intentEnableBluetooth = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                    enableBluetoothActivityResultLauncher.launch(intentEnableBluetooth);
                }
                else {
                    connectToArduino();
                }
            }
        });

        handler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg){
                switch (msg.what){
                    case CONNECTING_STATUS:
                        switch(msg.arg1){
                            case CONNECTING_STATUS_SUCCESS:
                                MainActivity.this.notify("Status", "Device connected");
                                progressBar.setVisibility(View.GONE);
                                bluethoothConnectButton.setEnabled(true);
                                forwardButton.setEnabled(true);
                                break;
                            case CONNECTING_STATUS_FAILURE:
                                MainActivity.this.notify("Status", "Device fails to connect");
                                progressBar.setVisibility(View.GONE);
                                bluethoothConnectButton.setVisibility(View.VISIBLE);
                                bluethoothConnectButton.setEnabled(true);
                                break;
                        }
                        break;

                }
            }
        };

    }

    private void connectToDevice(String address) {
        createConnectThread = new CreateConnectThread(bluetoothAdapter, deviceAddress);
        createConnectThread.start();
    }

    private class CreateConnectThread extends Thread {
        @SuppressLint("MissingPermission")
        public CreateConnectThread(BluetoothAdapter bluetoothAdapter, String address) {
            BluetoothDevice bluetoothDevice = null;
            try {
                bluetoothDevice = bluetoothAdapter.getRemoteDevice(address);
            } catch (IllegalArgumentException e) {
                handleError("Status", "Invalid address: " + address, e);
            }
            BluetoothSocket tmp = null;
            UUID uuid = bluetoothDevice.getUuids()[0].getUuid();

            try {
                /*
                Get a BluetoothSocket to connect with the given BluetoothDevice.
                Due to Android device varieties,the method below may not work fo different devices.
                You should try using other methods i.e. :
                tmp = device.createRfcommSocketToServiceRecord(MY_UUID);
                 */
                tmp = bluetoothDevice.createInsecureRfcommSocketToServiceRecord(uuid);

            } catch (IOException e) {
                Log.e(TAG, "Socket's create() method failed", e);
            }
            mmSocket = tmp;
        }

        @SuppressLint("MissingPermission")
        public void run() {
            // Cancel discovery because it otherwise slows down the connection.
            BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            bluetoothAdapter.cancelDiscovery();
            try {
                // Connect to the remote device through the socket. This call blocks
                // until it succeeds or throws an exception.
                mmSocket.connect();
//                MainActivity.this.notify("Status", "Device connected");
                handler.obtainMessage(CONNECTING_STATUS, CONNECTING_STATUS_SUCCESS, 0).sendToTarget();
            } catch (IOException connectException) {
                // Unable to connect; close the socket and return.
                try {
                    mmSocket.close();
                    handleError("Status", "Cannot connect to device", null);
                    handler.obtainMessage(CONNECTING_STATUS, CONNECTING_STATUS_FAILURE, CONNECTING_STATUS_FAILURE).sendToTarget();
                } catch (IOException closeException) {
                    Log.e(TAG, "Could not close the client socket", closeException);
                }
                return;
            }

            // The connection attempt succeeded. Perform work associated with
            // the connection in a separate thread.
            connectedThread = new ConnectedThread(mmSocket);
            connectedThread.run();
        }

        // Closes the client socket and causes the thread to finish.
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {
                Log.e(TAG, "Could not close the client socket", e);
            }
        }
    }
    /* =============================== Thread for Data Transfer =========================================== */
    public class ConnectedThread extends Thread {

        private final BluetoothSocket mmSocket;
        private final InputStream mmInStream;
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            // Get the input and output streams, using temp objects because member streams are final
            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {

        }

        /* Call this to send data to the remote device */
        public void writeCommand(Command command) {
            byte[] bytes = command.getBytes(); //converts entered String into bytes
            try {
                mmOutStream.write(bytes);
            } catch (IOException e) {
                handleError("Send Error","Unable to send message",e);
            }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) { }
        }
    }

    public void notify(String tag, String msg){
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
        Log.e(tag, msg);
    }

    public void handleError(String tag, String msg, Exception e){
//      Toast.makeText(MainActivity.this, msg, Toast.LENGTH_SHORT).show();
        Log.e(tag, msg, e);
    }

    @SuppressLint("MissingPermission")
    private void connectToArduino() {
         Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
        if (pairedDevices.size() > 0) {
            // There are paired devices. Get the name and address of each paired device.
            for (BluetoothDevice device : pairedDevices) {
                String deviceName = device.getName();
                if (deviceName.equals(ARDUINO_DEVICE_NAME)) {
                     deviceAddress = device.getAddress();
                     notify("Connection","Connecting to " + deviceName + "@"+ deviceAddress);
                     connectToDevice(deviceAddress);
                     return;
                }
            }
        }
        notify("Connection","Activate Bluetooth and pair Bluetooth device: " + ARDUINO_DEVICE_NAME);
    }

    @Override
    public void onBackPressed() {
        // Terminate Bluetooth Connection and close app
        if (createConnectThread != null){
            createConnectThread.cancel();
        }
        Intent a = new Intent(Intent.ACTION_MAIN);
        a.addCategory(Intent.CATEGORY_HOME);
        a.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(a);
    }
}
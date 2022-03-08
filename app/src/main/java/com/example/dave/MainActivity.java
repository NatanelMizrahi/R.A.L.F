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
import android.view.MotionEvent;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.TimePicker;
import android.widget.Toast;

import com.google.android.material.floatingactionbutton.FloatingActionButton;

import java.io.IOException;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private final static String ARDUINO_DEVICE_NAME = "HC-06";

    private BluetoothAdapter bluetoothAdapter;
    private String deviceAddress;

    public static Handler handler;
    public static BluetoothSocket mmSocket;
    public static ConnectedThread connectedThread;
    public static CreateConnectThread createConnectThread;

    private final static int CONNECTING_STATUS = 1;
    private final static int CONNECTING_STATUS_SUCCESS = 1;
    private final static int CONNECTING_STATUS_FAILURE = -1;
    private final static int CONNECTING_STATUS_TERMINATED = 0;

    private Map<ImageButton, Command> buttonCommandMap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TimePicker alarmTimePicker = findViewById(R.id.alarmTimePicker);
        final FloatingActionButton bluethoothConnectButton = findViewById(R.id.bluethoothConnectButton);
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        final TextView connectedText = findViewById(R.id.connectedText);

        final ImageButton forwardButton = findViewById(R.id.forwardButton);
        final ImageButton backButton = findViewById(R.id.backButton);
        final ImageButton leftButton = findViewById(R.id.leftButton);
        final ImageButton rightButton = findViewById(R.id.RightButton);

        final Switch modeToggleSwitch = findViewById(R.id.modeToggleSwitch);
        final Switch alarmToggleSwitch = findViewById(R.id.alarmToggleSwitch);

        modeToggleSwitch.setEnabled(false);
        alarmToggleSwitch.setEnabled(false);

        modeToggleSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                connectedThread.writeCommand(isChecked ?
                        Command.CreateSetModeAnarchyCommand() :
                        Command.CreateSetModeRemoteControlCommand()
                );
            }
        });

        buttonCommandMap = new HashMap<ImageButton, Command>();

        buttonCommandMap.put(forwardButton, Command.CreateMoveCommand(Command.Direction.FORWARD));
        buttonCommandMap.put(backButton, Command.CreateMoveCommand(Command.Direction.BACK));
        buttonCommandMap.put(leftButton, Command.CreateMoveCommand(Command.Direction.LEFT));
        buttonCommandMap.put(rightButton, Command.CreateMoveCommand(Command.Direction.RIGHT));

        for(Map.Entry<ImageButton, Command> buttonEntry : buttonCommandMap.entrySet()) {
            ImageButton button = buttonEntry.getKey();
            Command command = buttonEntry.getValue();
            button.setEnabled(false);

            button.setOnTouchListener(new View.OnTouchListener() {
                @Override
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_DOWN) {
                        connectedThread.writeCommand(command);
                    }
                    if (event.getAction() == MotionEvent.ACTION_UP) {
                        connectedThread.writeCommand(Command.CreateStopCommand());
                    }
                    return true;
                }
            });
        }

        alarmToggleSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                Command command = isChecked ?
                        Command.CreateSetAlarmCommand(
                                alarmTimePicker.getCurrentHour(),
                                alarmTimePicker.getCurrentMinute()
                        ):
                        Command.CreateDisableAlarmCommand();

                connectedThread.writeCommand(command);
                if (isChecked){
                    String hours = command.getValue() / 60 > 0 ? command.getValue() / 60 + "hours and " : "";
                    String minutes = command.getValue() % 60 + " minutes";
                    MainActivity.this.notify("ALARM_STATUS", "Alarm set for " + hours + minutes);
                }
            }
        });

        alarmTimePicker.setIs24HourView(true);
        alarmTimePicker.setEnabled(false);

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
                                toggleControls(true);
                                break;
                            case CONNECTING_STATUS_FAILURE:
                                MainActivity.this.notify("Status", "Device fails to connect");
                                progressBar.setVisibility(View.GONE);
                                toggleControls(false);
                                break;
                            case CONNECTING_STATUS_TERMINATED:
                                MainActivity.this.notify("Status", "Device connection closed");
                                toggleControls(false);
                                break;
                        }
                        break;
                }
            }

            private void toggleControls(boolean deviceConnected) {
                modeToggleSwitch.setEnabled(deviceConnected);
                alarmToggleSwitch.setEnabled(deviceConnected);
                alarmTimePicker.setEnabled(deviceConnected);
                bluethoothConnectButton.setEnabled(!deviceConnected);
                bluethoothConnectButton.setVisibility(deviceConnected ?  View.GONE: View.VISIBLE);
                connectedText.setVisibility(deviceConnected ? View.VISIBLE : View.GONE);
                for (ImageButton imageButton: buttonCommandMap.keySet()) {
                    imageButton.setEnabled(deviceConnected);
                }
            }
        };

    }

    private void connectToDevice(String address) {
        createConnectThread = new CreateConnectThread(bluetoothAdapter, address);
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
                // Get a BluetoothSocket to connect with the given BluetoothDevice.
                tmp = bluetoothDevice.createInsecureRfcommSocketToServiceRecord(uuid);
            } catch (IOException e) {
                Log.e(TAG, "Socket's create() method failed", e);
            }
            mmSocket = tmp;
        }

        @SuppressLint("MissingPermission")
        public void run() {
            // Cancel discovery because it otherwise slows down the connection.
            bluetoothAdapter.cancelDiscovery();
            try {
                mmSocket.connect(); //blocking call
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
        private final OutputStream mmOutStream;

        public ConnectedThread(BluetoothSocket socket) {
            mmSocket = socket;
            OutputStream tmpOut = null;

            // Get the output stream, using temp objects because member streams are final
            try {
                tmpOut = socket.getOutputStream();
            } catch (IOException e) { }

            mmOutStream = tmpOut;
        }

        public void run() { }

        /* Call this to send data to the remote device */
        public void writeCommand(Command command) {
            byte[] bytes = command.getBytes(); //converts entered String into bytes
            try {
                mmOutStream.write(bytes);
            } catch (IOException e) {
                handleError("Send Error","Unable to send message",e);
                cancel();
            }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                mmSocket.close();
                handler.obtainMessage(CONNECTING_STATUS, CONNECTING_STATUS_TERMINATED, CONNECTING_STATUS_TERMINATED).sendToTarget();
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
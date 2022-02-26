package com.example.dave;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothSocket;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.example.dave.R;

public class MainActivity extends AppCompatActivity {

    private BluetoothAdapter bluetoothAdapter;
    private String deviceName = null;
    private String deviceAddress;

    public static Handler handler;
    public static BluetoothSocket mmSocket;
//    public static ConnectedThread connectedThread;
    public static CreateConnectThread createConnectThread;

    private final static int CONNECTING_STATUS = 1; // used in bluetooth handler to identify message status
    private final static int MESSAGE_READ = 2; // used in bluetooth handler to identify message update

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Button bluethoothConnectButton = findViewById(R.id.bluethoothConnectButton);
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        final TextView connectedText = findViewById(R.id.connectedText);
        final ImageButton forwardButton = findViewById(R.id.forwardButton);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        // If a bluetooth device has been selected from SelectDeviceActivity
        deviceName = getIntent().getStringExtra("deviceName");
        if (deviceName != null){
            // Get the device address to make BT Connection
            deviceAddress = getIntent().getStringExtra("deviceAddress");
            // Show progress
            progressBar.setVisibility(View.VISIBLE);
            bluethoothConnectButton.setVisibility(View.INVISIBLE);
            bluethoothConnectButton.setEnabled(false);

            BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
            createConnectThread = new CreateConnectThread(bluetoothAdapter,deviceAddress);
            createConnectThread.start();
        }
    }


    private class CreateConnectThread extends Thread {
        public CreateConnectThread(BluetoothAdapter bluetoothAdapter, String deviceAddress) {
            super();
            throw new  UnsupportedOperationException();
        }
    }
}
package com.example.dave;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.os.Bundle;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.example.dave.R;

public class MainActivity extends AppCompatActivity {
    private BluetoothAdapter bluetoothAdapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final Button bluethoothConnectButton = findViewById(R.id.bluethoothConnectButton);
        final ProgressBar progressBar = findViewById(R.id.progressBar);
        final TextView connectedText = findViewById(R.id.connectedText);
        final ImageButton forwardButton = findViewById(R.id.forwardButton);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }
}
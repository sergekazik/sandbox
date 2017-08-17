package com.example.ring_sergie.test1;

import android.Manifest;
import android.app.ListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.RequiresApi;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.TextView;
import android.widget.Toast;

import java.io.IOException;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "skazik";
    private static final boolean START_SCAN = true;
    private static final boolean STOP_SCAN = false;
    private Button mButton;
    private TextView helloTextView;
    private TextView mTextView;
    private CheckBox mCheckBox;
    private BluetoothAdapter mBluetoothAdapter;
    private final static int REQUEST_ENABLE_BT = 1;
    private BluetoothDevice mDevice = null;
    private boolean bFound = false;
    public String msUUID = java.util.UUID.randomUUID().toString();
    private boolean bScanInProgress = false;

    private class ConnectThread extends Thread {
        private static final String TAG = "skazik-socket";
        public BluetoothSocket mmSocket = null;
        private final UUID MY_UUID = java.util.UUID.fromString(msUUID);

        public ConnectThread(BluetoothDevice device) {
            // Use a temporary object that is later assigned to mmSocket
            // because mmSocket is final.
            BluetoothSocket tmp = null;

            try {
                // Get a BluetoothSocket to connect with the given BluetoothDevice.
                // MY_UUID is the app's UUID string, also used in the server code.
                tmp = device.createRfcommSocketToServiceRecord(MY_UUID);
            } catch (IOException e) {
                Log.e(TAG, "Socket's create() method failed", e);
            }
            mmSocket = tmp;
        }

        public void run() {
            try {
                // Connect to the remote device through the socket. This call blocks
                // until it succeeds or throws an exception.
                mmSocket.connect();
            } catch (IOException connectException) {
                // Unable to connect; close the socket and return.
                debugout("Unable to connect, exception " + connectException.toString() + "\r\n" + "close the socket and return.");
                try {
                    mmSocket.close();
                } catch (IOException closeException) {
                    Log.e(TAG, "Could not close the client socket", closeException);
                }
                return;
            }
            debugout("connected - manageMyConnectedSocket");

            // The connection attempt succeeded. Perform work associated with
            // the connection in a separate thread.
            manageMyConnectedSocket(mmSocket);
        }

        private void manageMyConnectedSocket(BluetoothSocket mmSocket) {
            Toast.makeText(getBaseContext(), "CONNECTED!!!", Toast.LENGTH_SHORT).show();
            try {
                mmSocket.close();
            } catch (IOException closeException) {
                Log.e(TAG, "Could not close the client socket", closeException);
            }
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

    public void debugout(String text)
    {
        Log.d(TAG, text);
        helloTextView.setText(text);
    }

    private void handleFound(BluetoothDevice device)
    {
        String deviceName = device.getName() != null ? device.getName() : "UNKNOWN";
        String deviceHardwareAddress = device.getAddress(); // MAC address

        String text = deviceName;
        text += "\r\n";
        text += deviceHardwareAddress;
        debugout(text);

        if (!bFound && deviceName.contains("Ampak"))
        {
            scan_discover(STOP_SCAN);
            bFound = true;
            mDevice = device;
            mButton.setText("Connect");
            mTextView.setText(text);
        }

    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            switch (action)
            {
                case BluetoothAdapter.ACTION_STATE_CHANGED: debugout("ACTION_STATE_CHANGED"); break;
                case BluetoothDevice.ACTION_FOUND: debugout("ACTION_FOUND"); break;
                case BluetoothAdapter.ACTION_DISCOVERY_STARTED: debugout("ACTION_DISCOVERY_STARTED"); break;
                case BluetoothAdapter.ACTION_DISCOVERY_FINISHED: debugout("ACTION_DISCOVERY_FINISHED"); break;
            }

            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                // Discovery has found a device. Get the BluetoothDevice
                // object and its info from the Intent.
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                handleFound(device);            }
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });
        mButton = (Button) findViewById(R.id.button1);
        helloTextView = (TextView) findViewById(R.id.text_view_id);
        mTextView =  (TextView) findViewById(R.id.textView1);
        mCheckBox = (CheckBox) findViewById(R.id.checkBox1);

        // Bluetooth section
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "Device does not support Bluetooth", Toast.LENGTH_SHORT).show();
        }

        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        final int MY_PERMISSIONS_REQUEST_ACCESS_COARSE_LOCATION = 1;
        ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.ACCESS_COARSE_LOCATION},
                MY_PERMISSIONS_REQUEST_ACCESS_COARSE_LOCATION);

        // check for BLE
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "ble_not_supported", Toast.LENGTH_SHORT).show();
        }

        // Register for broadcasts when a device is discovered.
        IntentFilter filter = new IntentFilter(); // (BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        filter.addAction(BluetoothDevice.ACTION_FOUND);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_STARTED);
        filter.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);

        registerReceiver(mReceiver, filter);

    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    @Override
    protected void onDestroy() {
        super.onDestroy();

        scan_discover(STOP_SCAN);
        // Don't forget to unregister the ACTION_FOUND receiver.
        unregisterReceiver(mReceiver);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    // Device scan callback.
    private BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {
                @Override
                public void onLeScan(final BluetoothDevice device, int rssi,
                                     byte[] scanRecord) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            handleFound(device);
                        }
                    });
                }
            };

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void scan_discover(boolean bStart)
    {
        if (bStart && !bScanInProgress)
        {
            bScanInProgress = true;
            if (mCheckBox.isChecked()) {
                mBluetoothAdapter.startLeScan(mLeScanCallback);
            }
            else {
                mBluetoothAdapter.startDiscovery();
            }

            mButton.setText("scanning... Click to stop");
        }
        else if (!bStart && bScanInProgress)
        {
            bScanInProgress = false;
            if (mCheckBox.isChecked()) {
                mBluetoothAdapter.stopLeScan(mLeScanCallback);
            }
            else
            {
                mBluetoothAdapter.cancelDiscovery();
            }
            mButton.setText(bFound ? "Connect" : "Scan");
        }
        mCheckBox.setEnabled(!bScanInProgress);
    }


    public void OnCheckBoxClick(View v) {
        bFound = false;
        mButton.setText("Scan");
    }

    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN_MR2)
    public void OnButtonClick(View v) {

        if (!bFound) {
            if (bScanInProgress) {
                scan_discover(STOP_SCAN);
            } else {
                scan_discover(START_SCAN);
            }
        }
        else
        {
            debugout("trying to connect " + mDevice.getAddress() + "\r\n" + msUUID);
            if (mDevice != null) {
                ConnectThread mConnect = new ConnectThread(mDevice);
                if (mConnect.mmSocket != null) {
                    mConnect.run();
                }
            }
            else
            {
                debugout("mDevice == null");
            }
        }
    }

}
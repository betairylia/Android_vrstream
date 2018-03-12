package com.android.gl2jni;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.widget.CheckBox;
import android.widget.EditText;
import android.view.View;

import java.nio.ByteBuffer;

public class ConnectionActivity extends AppCompatActivity implements View.OnClickListener
{
    Button btn;
    EditText ipaddrTxt, portTxt;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connection);

        ipaddrTxt = (EditText) findViewById(R.id.eTxt_ipaddr);
        portTxt = (EditText) findViewById(R.id.eTxt_port);

        btn = (Button) findViewById(R.id.btn_connect);
        btn.setOnClickListener(this);
    }

    @Override
    public void onClick(View view)
    {
        Snackbar.make(view, "Connecting...", Snackbar.LENGTH_LONG)
                .setAction("Action", null).show();

        GL2JNILib.InitSocket(ipaddrTxt.getText().toString(), Integer.parseInt(portTxt.getText().toString()));

        //Start another activity
        Intent intent = new Intent(this, GL2JNIActivity.class);
        startActivity(intent);
    }
}

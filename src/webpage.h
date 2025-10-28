#ifndef _WEBPAGE_H_
#define _WEBPAGE_H_

char idx[] PROGMEM = R"=====(


<!DOCTYPE html>
<html lang="en">
    

<head>
    <meta charset="utf-8" http-equiv="connection:Keep-Alive">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="content-type" content="text/html;charset=utf-8" />
    <title>Topení</title>
</head>
<body style="background-color: blue;">
    <style>
        body {
            
            display: flex;
            justify-content: center;
            align-items: flex-start;
            height: 100vh;
            margin: 0;
        }

        table {
            background-color: aliceblue;
            text-align: center;
            border: 1px solid #ccc;
            padding: 5px;
            border-radius: 10px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
        }

        input{
            width: 50px;
            height: 40px;
            font-size: large;
            font-weight: bold;
            text-align: center;
        }

        .dot {
             height: 40px;
             width: 40px;
             background-color: gray;
             border-radius: 50%;
             display: inline-block;
             margin-left: 12px;
             
        }
    </style>
    <table id="mainTbl"  width="auto" style="margin-top: 100px;">
        <tr>
            <td style="margin: auto; width: 50%;">
                <!-- Existing table starts here -->
                <table style="background-color: rgb(159, 219, 233);">
                    <tr>
                        <td><label for="vymenik">vyměník:</label></td>
                        <td><input type="text" id="vymenik" name="vymenik" readonly></td>
                    </tr>
                    <tr>
                        <td><label for="stoupacka">stoupačka:</label></td>
                        <td><input type="text" id="stoupacka" name="stoupacka" readonly></td>
                    </tr>
                    <tr>
                        <td><label for="zpatecka">zpátečka:</label></td>
                        <td><input type="text" id="zpatecka" name="zpatecka" readonly></td>
                    </tr>
                    <tr>
                        <td><label for="krb">krb:</label></td>
                        <td><input type="text" id="krb" name="krb" readonly></td>
                    </tr>
                    <tr>
                        <td><label for="mistnost">místnost:</label></td>
                        <td><input type="text" id="mistnost" name="mistnost" readonly></td>
                    </tr>
                                        <tr>
                        <td><label for="komin1">komín1:</label></td>
                        <td><input type="text" id="komin1" name="komin1" readonly></td>
                    </tr>
                    <tr>
                        <td><label for="komin2">komín2:</label></td>
                        <td><input type="text" id="komin2" name="komin2" readonly></td>
                    </tr>
                </table>
                <!-- Existing table ends here -->
            </td>
            <td style="margin: auto; width: 50%;">
                <table style="margin: auto; width: 100%; background-color: rgb(159, 219, 233);" >
                    <tr>
                        <td><label for="switchON">teplota sepnutí:</label></td>
                        <td><input type="number" id="switchON" name="switchON" min="40" max="60"></td>
                    </tr>
                    <tr>
                        <td><label for="switchOFF">teplota vypnutí:</label></td>
                        <td><input type="number" id="switchOFF" name="switchOFF" min="10" max="40"></td>
                    </tr>
                    <tr>
                        <td><label for="alarmTemp">teplota alarm:</label></td>
                        <td><input type="number" id="alarmTemp" name="alarmTemp" min="70" max="85"></td>
                    </tr> 
                </table>
                <table style="margin-top: 5px; width: 100%; background-color: rgb(159, 219, 233);">
                    <tr>
                        <td><label >čerpadlo:</label></td>
                        <td><span id="signalCerp" class="dot"></span></td>
                    </tr>
                </table>
                <table style="margin-top: 5px; width: 100%; background-color: rgb(159, 219, 233);">
                    <tr>
                        <td><label>alarm:</label></td>
                        <td rowspan="2" ><span id="signalAlarm" class="dot"></span></td>
                    </tr>
                    <tr>
                        <td >
                            <button type="button" id="rstBtn">RESET</button>
                        </td>
                        <td></td>
                        
                    </tr>
                </table>
            </td>
        </tr>
        <tr>       
            <td>
                <button type="button" id="saveBtn">Ulož</button>
            </td>      
        </tr>
    </table>

<script>


    const ws = new WebSocket(((window.location.protocol === "https:") ? "wss://" : "ws://") + window.location.host + "/ws");

    ws.onmessage = function(event) {
        // Example message: "komin1>12>komin2>34>vymenik>56>stoupacka>78>zpatecka>90>krb>21>mistnost>43"
        const data = event.data.split('>');
        for (let i = 0; i < data.length - 1; i += 2) {
            const key = data[i];
            const value = data[i + 1];
            const input = document.getElementById(key);
            if (input) input.value = value;
            if(key == "alarm"){
               if(value == "on")
               {
                document.getElementById('signalAlarm').style.backgroundColor = "red";
                document.getElementById('mainTbl').style.backgroundColor = "red";
               }
               else if(value == "off")
               {
                document.getElementById('signalAlarm').style.backgroundColor = "gray";
                document.getElementById('mainTbl').style.backgroundColor = "aliceblue";
               }
            }
            if(key == "cerpadlo"){
               if(value == "on")document.getElementById('signalCerp').style.backgroundColor = "yellowgreen";
               else if(value == "off")document.getElementById('signalCerp').style.backgroundColor = "gray";                
            }
        }
    };

document.getElementById('saveBtn').addEventListener('click', function() {
    const fields = ['switchON', 'switchOFF', 'alarmTemp'];
    let msg = 'set>';
    fields.forEach(f => {
        const val = document.getElementById(f).value;
        msg += val;
        msg  += '>';
    });
    ws.send(msg);
});


document.getElementById('rstBtn').addEventListener('click', function() {

    ws.send("rst");

});

['switchON', 'switchOFF', 'alarmTemp'].forEach(id => {
    document.getElementById(id).addEventListener('input', (event) => {
        console.log(`${id} value changed to:`, event.target.value);
        if (event.target.value > event.target.max) event.target.value = event.target.max;
        if (event.target.value < event.target.min) event.target.value = event.target.min;
    });
});
</script>


</body>

</html> 
)=====";


#endif
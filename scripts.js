function Click(element) {
    if (element.className == 'btn1') {
        if (element.style.backgroundColor == 'lightgreen') element.style.backgroundColor = 'green';
        else element.style.backgroundColor = 'lightgreen';
    }
    else {
        if (element.style.backgroundColor == 'yellow') element.style.backgroundColor = 'darkgoldenrod';
        else element.style.backgroundColor = 'yellow';
    }

}
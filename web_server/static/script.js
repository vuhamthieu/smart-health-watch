function updateData() {
    fetch('/get_data')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            const hr = data.heart_rate ?? 0;
            const sp = data.spo2 ?? 0;
            const tp = data.temperature ?? 0;
            document.getElementById('heart_rate').textContent = `${hr} bpm`;
            document.getElementById('spo2').textContent = `${sp} %`;
            document.getElementById('temperature').textContent = `${Number(tp).toFixed(2)} °C`;
            const status = (data._last_ts && (Date.now()/1000 - data._last_ts) < 5) ? 'Connected' : 'Disconnected';
            const el = document.getElementById('connection_status');
            if (el) el.textContent = status;
        })
        .catch(error => console.error('Error fetching data:', error));
}
setInterval(updateData, 5000);
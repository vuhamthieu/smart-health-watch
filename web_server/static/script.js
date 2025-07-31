function updateData() {
    fetch('/get_data')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! Status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            document.getElementById('heart_rate').textContent = data.heart_rate || 0;
            document.getElementById('spo2').textContent = data.spo2 || 0;
            document.getElementById('temperature').textContent = data.temperature || 0;
        })
        .catch(error => console.error('Error fetching data:', error));
}
setInterval(updateData, 5000);
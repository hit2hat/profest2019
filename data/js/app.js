const ends = {
    "temperature": "℃",
    "humidity": "%",
    "fuel": "%"
};

const App = () => {
    fetch("/api/getMetrics")
    .then((data) => data.json())
    .then((data) => {
        Object.keys(data).forEach((key) => {
            var elem = document.getElementById(key);
            if (elem !== null)
                elem.innerHTML = ends[key] ? data[key] + ends[key] : data[key];
        })
        setTimeout(App, 1000);
    })
    .catch((error) => {
        console.log(error);
        setTimeout(App, 1000);
    });
};

App();
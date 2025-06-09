## About

`elspot` is a small C++20 application that will fetch electricity spot-prices from [Entso-E](https://www.entsoe.eu) for the Norwegian zones NO-1 to NO-5 (as Nordpool for whatever reason does not allow crawling for this automatically) and publishes the prices in SVG format, as Grafana dashboards and MQTT topics.  
Tests are using Google Test and code coverage is using lcov. There are [GitHub Actions](https://github.com/frodegill/elspot/tree/main/.github/workflows) for tests and code quality.


## Links

SVGs of today spotprice for zone NO-1 to NO-5 in [Norwegian Krone](https://gillhub.org/index.php?view=article&id=5&catid=21) or [Euro](https://gillhub.org/index.php?view=article&id=4&catid=21)  
SVGs of tomorrow spotprice for zone NO-1 to NO-5 in [Norwegian Krone](https://gillhub.org/index.php?view=article&id=6&catid=21) or [Euro](https://gillhub.org/index.php?view=article&id=7&catid=21) (available from around 1400UTC)  

Grafana dashboard for spotprice in [Norwegian Krone](https://gillhub.org:3000/d/mGElKXi7k/spotprice-nok?orgId=1&from=now-30d&to=now) or [Euro](https://gillhub.org:3000/d/ghKaZXz7z/spotprice-eur?orgId=1&from=now-30d&to=now)  

A page with all [MQTT information you need](https://gillhub.org/index.php?view=article&id=8&catid=21) for fetching spot-prices from my server
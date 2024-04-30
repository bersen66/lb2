1. [Introduction](#lb2)
    1. [Supported algorithms](#supported-algorithms)
2. [Installation](#installation)
3. [YAML-config example](#config-example)
4. [Run app](#run-app)

# lb2

lb2 is an asynchronous multithreaded load balancer written in C++. It is designed to be highly configurable and supports six different algorithms of load balancing. This project aims to provide a flexible and efficient solution for distributing network traffic across multiple servers.

## Supported algorithms
* `round-robin`
* `weighted round-robin`
* `ip-hash`
* `consistent hash`
* `least-connections`
* `least-response-time`

# Installation:

First of all make sure, that `pip` is installed. It will be used for installing `CMake` and `Conan` if needed.

> To automatically install all dependencies and build application type:

```bash
./lbbuild.sh build Release
```

This script builds app and copies `configs` folder to ``` build/lb2/configs```

> To build and run unit tests type:

```bash
./lbbuild.sh test Release
```

# Config example:

```yaml
# Configure logging
logging:
  # Optional value.
  file:
    name: logs/logs.txt # Logfile name
    level: debug  # Possible values: trace, debug, info, warning, error
  console:
    level: debug

thread_pool:
  threads_number: auto # or number

acceptor:
  port: 9090  # Port number
  ip_version: 4 # or 6

# Configure load balancing algorithm
load_balancing:
  # Possible values:
  #  - round_robin
  #  - weighted_round_robin
  #  - consistent_hash
  #  - ip_hash
  #  - least_connections,
  #  - least_response_time


  # Example config of least connections
  algorithm: least_connections
  endpoints:
    - ip: "127.0.0.1"
      port: 8081
    - ip: "127.0.0.2"
      port: 8082
    - ip: "127.0.0.3"
      port: 8083

  # Example config of least response time
  # algorithm: least_response_time
  # endpoints:
  #   - ip: "127.0.0.1"
  #     port: 8081
  #   - ip: "127.0.0.2"
  #     port: 8082
  #   - ip: "127.0.0.3"
  #     port: 8083

  # Example config of round robin
  # algorithm: round_robin
  # endpoints:
  #   - ip: "127.0.0.1"
  #     port: 8081
  #   - ip: "127.0.0.2"
  #     port: 8082
  #   - ip: "127.0.0.3"
  #     port: 8083

  # Example config of weighted round robin
  # algorithm: weighted_round_robin
  # endpoints:
  #   - ip: "127.0.0.1"
  #     port: 8081
  #     weight: 1
  #   - ip: "127.0.0.2"
  #     port: 8082
  #     weight: 2
  #   - ip: "127.0.0.3"
  #     port: 8083
  #     weight: 3

  # Example config of consistent hash
  # algorithm: consistent_hash
  # replicas: 5
  # endpoints:
  #   - ip: "127.0.0.1"
  #     port: 8081
  #   - ip: "127.0.0.2"
  #     port: 8082
  #   - ip: "127.0.0.3"
  #     port: 8083

  # Example config of ip hash
  # algorithm: ip_hash
  # endpoints:
  #   - ip: "127.0.0.1"
  #     port: 8081
  #   - ip: "127.0.0.2"
  #     port: 8082
  #   - ip: "127.0.0.3"
  #     port: 8083
```

# Run app

* Firstly, build app. To do it type: ```./lbbuild.sh build Release```

* Then, go to ```build/lb2/```

* Modify configs in ```build/lb2/configs/config.yaml``` or write your own.

* Run app using: ```./lb_app```



# IRC Watchlist Bot
### A Dockerized IRC Bot written in C for checking messages towards various nations' watchlists.
***

### Setting up

##### Quickstart

**Build and run using Docker**

Make sure you meet the following prerequisites:
* `docker` is installed locally or configured to use another server

```Bash
# Clone the repository (or download it)
git clone git@github.com:AlexGustafsson/irc-watchlist-bot.git

# Build the wsic docker image
cd wsic && make docker

# Run
docker run -d \
-e IRC_SERVER='irc.example.org' \
-e IRC_PORT='6697' \
-e IRC_CHANNEL='#random' \
-e IRC_NICK='watchlist-bot' \
-e IRC_USER='watchlist-bot' \
-e IRC_GECOS='Watchlist Bot v0.1.2 (github.com/AlexGustafsson/irc-watchlist-bot)' \
axgn/irc-watchlist-bot
```

**Build and run from source**

Make sure you meet the following prerequisites:
* `$CC` refers to `gcc` 9 (`brew install gcc` on macOS) or `clang` 7
* `xxd` is installed (default on many distributions)
* `gnu sed` is installed and available as `sed` (default on many distributions, `brew install gnu-sed` on macOS)
* `openssl 1.1.1` is available in the system include path or `/usr/local/opt/openssl@1.1/include` (`apt install libssl-dev` on Ubuntu, `brew install openssl@1.1` on macOS)

_NOTE: For instructions on how to install all the prerequisites and building on Ubuntu, refer to the [Dockerfile](https://github.com/AlexGustafsson/irc-watchlist-bot/blob/master/Dockerfile)._

```Bash
# Clone the repository (or download it)
git clone git@github.com:AlexGustafsson/irc-watchlist-bot.git

# Build wsic
cd irc-watchlist-bot && make

# Run
./build/irc-watchlist-bot
```

```Bash
# Clone the repository
git clone https://github.com/AlexGustafsson/irc-watchlist-bot
# Enter the directory
cd irc-watchlist-bot
# Build the image
make docker
# Start the image
docker run -d -e IRC_SERVER='irc.example.org' --restart always axgn/irc-watchlist-bot
```

### Documentation

#### Invoking via IRC

To see help messages send `watchlist-bot: help` in the channel where the bot lives.

The bot reads all messages sent in the configured channel and sends an appropriate response if a message is on a watchlist.

### Contributing

Any contribution is welcome. If you're not able to code it yourself, perhaps someone else is - so post an issue if there's anything on your mind.

###### Development

Clone the repository:
```
# Clone the repository
git clone https://github.com/AlexGustafsson/irc-watchlist-bot && cd irc-watchlist-bot

# Format the code
make format

# Build and run a debugging build (memory analyzer and GDB debugging enabled)
make debug && ASAN_OPTIONS=detect_leaks=1 ./build/irc-watchlist-bot.debug

# Build and run a release build
make build && ./irc-watchlist-bot
```

### Disclaimer

_Although the project is very capable, it is not built with production in mind. Therefore there might be complications when trying to use the bot for large-scale projects meant for the public. The bot was created to easily check messages towards nations' watchlists in IRC channels and as such it might not promote best practices nor be performant._

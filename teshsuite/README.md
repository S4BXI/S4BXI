# Test suite

To compile / run every test, simply use `./run.sh` (**from this directory**), it will :
* Use cmake and make to build each directory starting with an undescore
* Use tesh to run every test (which should have the extension `*.tesh`)

To delete all the built file for each test, use `./clean.sh`, it will delete the `build` directory in each test folder (starting with an underscore)
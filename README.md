
# CSE 535 - Project 2: PBFT

### Project Details
- **Name**: Kunal Chadha
- **SBU ID**: 116714323

### Tests
The test cases for the system are provided in the file: `tests/test-1.csv`.

---

## Running the System

### Option 1: Setup Using Docker

To quickly set up and run the system using Docker, follow these steps:

1. **Install Docker**: Follow the instructions here to install Docker: [Get Docker](https://docs.docker.com/get-started/get-docker/).
   
2. **Run the Docker setup script**:
   ```bash
   docker-compose up --build
   ```

3. **Access the `apaxos-client` container**:
   ```bash
   docker exec -it linear-pbft-client /bin/bash
   ```

4. **Run the client program**:
   ```bash
   ./linear-pbft-client
   ```
   

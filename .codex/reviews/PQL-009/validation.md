# PQL-009: Validation report

## Acceptance criteria

- PASS: compose.yaml defines Docker Compose configuration.
- PASS: exactly one service exists, named postgres.
- PASS: SQL confirmed database personal_quant_lab and local bootstrap user pql.
- PASS: no Redis, Kafka or other service was added.

## Configuration evidence

Root and independent Validator ran:

```powershell
docker compose config --quiet
docker compose config --services
docker compose --env-file .env.example config --quiet
```

Valid local configuration passed and services output contained only postgres. The
blank-password template correctly failed required-variable interpolation. Git checks
confirmed .env is ignored and untracked while .env.example is eligible for tracking.
The generated local password was never printed or committed. Tracked whitespace
validation passed.

## Runtime evidence from root

Docker 29.4.1 and Compose v5.1.3 were installed. Docker Desktop was initially stopped;
root started it in the background. Access to the user's Docker context and engine
required approved execution outside the filesystem sandbox.

```powershell
docker compose up -d --wait --wait-timeout 90 postgres
docker compose ps
docker compose exec -T postgres psql -U pql -d personal_quant_lab -v ON_ERROR_STOP=1 -Atc 'SELECT current_database(), current_user;'
```

The official postgres:17 image was pulled. Compose created its project network,
personal-quant-lab_postgres_data volume and personal-quant-lab-postgres-1 container.
The service became healthy and published only 127.0.0.1:5432. The SQL result was
`personal_quant_lab|pql`.

Root additionally piped the same SQL into psql using TCP host postgres and the
container's POSTGRES_PASSWORD as PGPASSWORD (without displaying it). The query again
returned `personal_quant_lab|pql`. A host TCP socket connected to 127.0.0.1:5432.

Persistence was checked by reading pg_control_system().system_identifier before and
after `docker compose up -d --force-recreate --wait --wait-timeout 60 postgres`. The
identifier remained identical, the requested database remained available, and the
replacement container became healthy. No application tables or test data were added.

## Scope, state and remaining debt

PostgreSQL is left running. Data stays in its named volume. Ordinary `docker compose
down` removes the container/network while retaining the volume. Initialization
credentials apply only to an empty volume, as documented by the official image.

The independent Validator verified static/configuration acceptance and used root's
runtime evidence without repeating container operations. C++ tests were not rerun:
this ticket changes only local infrastructure and setup documentation. Engine/database
integration, schemas, migrations, backup/restore and cross-platform provisioning were
not implemented or verified here.

## Decision and sources

**VALIDATION PASSED**, with no blocking review findings.

- [Official PostgreSQL image](https://hub.docker.com/_/postgres): initialization variables and PostgreSQL 17 data-volume path.
- [Compose up](https://docs.docker.com/reference/cli/docker/compose/up/): readiness waiting.

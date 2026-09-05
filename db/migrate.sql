\set ON_ERROR_STOP on
BEGIN;
-- Serialize migration runners; history and DDL commit together.
SELECT pg_advisory_xact_lock(710010);
CREATE TABLE IF NOT EXISTS public.schema_migrations (
    version integer PRIMARY KEY CHECK (version > 0),
    description text NOT NULL CHECK (btrim(description) <> ''),
    applied_at timestamptz NOT NULL DEFAULT CURRENT_TIMESTAMP CHECK (isfinite(applied_at))
);
SELECT EXISTS (SELECT 1 FROM public.schema_migrations WHERE version = 1) AS applied \gset
\if :applied
    \echo 'Migration 001 already applied'
\else
    \ir migrations/001_v01_schema.sql
    INSERT INTO public.schema_migrations(version, description)
    VALUES (1, 'PQL-010 v0.1 financial schema');
\endif
COMMIT;

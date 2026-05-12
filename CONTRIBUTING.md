# Contributing to macosx-security-patches

Thanks for helping with legacy Mac OS X security patch documentation and
experiments. This repository is safety-sensitive: changes may affect old systems
that are hard to recover, so clear validation and rollback notes are required.

## Useful Contributions

- Improve installation, backup, and rollback instructions.
- Add verified notes for specific Panther, Tiger, Leopard, or Snow Leopard
  hardware combinations.
- Clarify CVE descriptions, affected components, and mitigation limits.
- Add test plans for non-production machines.
- Fix broken links, package names, or confusing command examples.

## Development Workflow

1. Fork the repository and create a focused branch.
2. Keep one CVE, patch, or documentation area per PR when possible.
3. Cite CVE records, advisories, or upstream source material for security
   claims.
4. Include backup, test, and rollback details for installer or patch changes.

## Validation

- Documentation-only changes: run `git diff --check`.
- Installer or script changes: test on a non-production system or VM and include
  the exact OS version and command output.
- Security claims: include the CVE identifier and source used.

## Pull Request Checklist

- The affected OS version and architecture are named.
- The PR describes the validation environment.
- Backup and rollback guidance is included for risky changes.
- Security claims cite public references.
- Generated packages or machine-specific files are not committed accidentally.

## Reporting Issues

Include OS version, architecture, Mac model, patch or installer name, install
command, full error output, and whether a backup or rollback was attempted.
For crash reports, include only the relevant panic/log lines.

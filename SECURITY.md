# Security Policy
As an open-source project, we understand the importance of and responsibility
for security. This Security policy outlines our guidelines and procedures for
ensuring the highest level of Security and trust for our users who consume
oneTBB.

## Supported Versions
We regularly perform patch releases for the supported
[latest version][1],
which contain fixes for security vulnerabilities and important bugs. Prior
releases might receive critical security fixes on a best-effort basis; however,
we cannot guarantee that security fixes will get back-ported to these
unsupported versions.

## Report a Vulnerability
We are very grateful to the security researchers and users that report back
security vulnerabilities. We investigate every report thoroughly.
We strongly encourage you to report security vulnerabilities to us privately,
before disclosing them on public forums or opening a public GitHub issue. 
Report a vulnerability to us in one of two ways:
* Open a draft [**GitHub Security Advisory**][2]
* Send e-mail to the following address: **security@lists.uxlfoundation.org**.
Along with the report, please include the following info:
  * A descriptive title.
  * Your name and affiliation (if any).
  * A description of the technical details of the vulnerabilities.
  * A minimal example of the vulnerability so we can reproduce your findings.
  * An explanation of who can exploit this vulnerability, and what they gain
  when doing so. 
  * Whether this vulnerability is public or known to third parties. If it is,
  please provide details.

### When Should I Report a Vulnerability?
* You think you discovered a potential security vulnerability in oneTBB.
* You are unsure how the potential vulnerability affects oneTBB.
* You think you discovered a vulnerability in another project or 3rd party
component on which oneTBB depends. If the issue is not fixed in the 3rd party
component, try to report directly there first.

### When Should I NOT Report a Vulnerability?
* You got an automated scan hit and are unable to provide details.
* You need help using oneTBB for security.
* You need help applying security-related updates.
* Your issue is not security-related.

## Security Reports Review Process
Our goal is to respond quickly to your inquiry, and to coordinate a fix and
disclosure with you. All confirmed security vulnerabilities will be addressed
according to severity level and impact on oneTBB. Normally, security issues
are fixed in the next planned release.

## Disclosure Policy
We will publish security advisories using the 
[**GitHub Security Advisories feature**][3]
to keep our community well-informed, and will credit you for your findings
unless you prefer to stay anonymous. We request that you refrain from
exploiting the vulnerability or making it public before the official disclosure.

We will disclose the vulnerabilities and/or bugs as soon as possible once
mitigation is implemented and available. 

## Feedback on This Policy
If you have any suggestions on how this Policy could be improved, please submit
an issue or a pull request to this repository. Please **do not** report
potential vulnerabilities or security flaws via a pull request.

[1]: https://github.com/oneapi-src/oneTBB/releases/latest
[2]: https://github.com/oneapi-src/oneTBB/security/advisories/new
[3]: https://github.com/oneapi-src/oneTBB/security/advisories

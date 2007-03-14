/*
  Copyright (c) 2007 by Jakob Schroeter <js@camaya.net>
  This file is part of the gloox library. http://camaya.net/gloox
  This software is distributed under a license. The full license
  agreement can be found in the file LICENSE in this distribution.
  This software may not be copied, modified, sold or distributed
  other than expressed in the named license agreement.
  This software is distributed without any warranty.
*/

#include "tlsschannel.h"

#include <stdio.h> // just for debugging output
#include <iostream>

namespace gloox {
SChannel::SChannel( TLSHandler *th, const std::string& server ) :
    TLSBase( th, server ) {
        printf(">> SChannel::SChannel()\n");
    }

    SChannel::~SChannel() {
        printf(">> SChannel::~SChannel()\n");
    }

    bool SChannel::encrypt( const std::string& data ) {
        printf(">> SChannel::encrypt()\n");
        std::string data_copy = data;

        SecBuffer       buffer[4];
        SecBufferDesc   buffer_desc;
        DWORD cbIoBufferLength = m_sizes.cbHeader + m_sizes.cbMaximumMessage + m_sizes.cbTrailer;
        char str[50];
        sprintf(str, "MAXMSG: %i", m_sizes.cbMaximumMessage);
        OutputDebugString(str);

        PBYTE e_iobuffer = static_cast<PBYTE>(LocalAlloc(LMEM_FIXED, cbIoBufferLength));
        if (e_iobuffer == NULL) {
            printf("**** Out of memory (2)\n");
            return 1;
        }
        PBYTE e_message = e_iobuffer + m_sizes.cbHeader;
        do {
            std::string tmp = data_copy.substr(0, data_copy.size() > m_sizes.cbMaximumMessage ? m_sizes.cbMaximumMessage : data_copy.size());
            if (data_copy.size() > m_sizes.cbMaximumMessage) {
                data_copy = data_copy.substr(m_sizes.cbMaximumMessage-1);
            }

            // copy data chunk from tmp string into encryption memory buffer
            sprintf(str, "tmp.size(): %i", tmp.size());
            OutputDebugString(str);
            memcpy(e_message, tmp.data(), tmp.size());

            buffer[0].pvBuffer     = e_iobuffer;
            buffer[0].cbBuffer     = m_sizes.cbHeader;
            buffer[0].BufferType   = SECBUFFER_STREAM_HEADER;

            buffer[1].pvBuffer     = e_message;
            buffer[1].cbBuffer     = tmp.size();
            buffer[1].BufferType   = SECBUFFER_DATA;

            buffer[2].pvBuffer     = static_cast<char*>(buffer[1].pvBuffer) + buffer[1].cbBuffer;
            buffer[2].cbBuffer     = m_sizes.cbTrailer;
            buffer[2].BufferType   = SECBUFFER_STREAM_TRAILER;

            buffer[3].BufferType   = SECBUFFER_EMPTY;

            buffer_desc.ulVersion       = SECBUFFER_VERSION;
            buffer_desc.cBuffers        = 4;
            buffer_desc.pBuffers        = buffer;

            SECURITY_STATUS e_status = EncryptMessage(&m_context, 0, &buffer_desc, 0);
            if (SUCCEEDED(e_status)) {
                std::string encrypted(reinterpret_cast<const char*>(e_iobuffer), buffer[0].cbBuffer + buffer[1].cbBuffer + buffer[2].cbBuffer);
                m_handler->handleEncryptedData(encrypted);
                if (data_copy.size() <= m_sizes.cbMaximumMessage) data_copy = "";
            } else {
                LocalFree(e_iobuffer);
                // throw an error
                return 1;
            }
        } while (data_copy.size() > 0);
        LocalFree(e_iobuffer);
        return 0;
    }

    int SChannel::decrypt( const std::string& data ) {
        printf(">> SChannel::decrypt()\n");
        if (m_secure) {
            m_buffer += data;

            SecBuffer       buffer[4];
            SecBufferDesc   buffer_desc;
            DWORD cbIoBufferLength = m_sizes.cbHeader + m_sizes.cbMaximumMessage + m_sizes.cbTrailer;

            PBYTE e_iobuffer = static_cast<PBYTE>(LocalAlloc(LMEM_FIXED, cbIoBufferLength));
            if (e_iobuffer == NULL) {
                printf("**** Out of memory (2)\n");
                return 1;
            }
            SECURITY_STATUS e_status;

            // copy data chunk from tmp string into encryption memory buffer
            do {
                memcpy(e_iobuffer, m_buffer.data(), m_buffer.size() > cbIoBufferLength ? cbIoBufferLength : m_buffer.size());

                buffer[0].pvBuffer     = e_iobuffer;
                buffer[0].cbBuffer     = m_buffer.size() > cbIoBufferLength ? cbIoBufferLength : m_buffer.size();
                buffer[0].BufferType   = SECBUFFER_DATA;

                buffer[1].BufferType = buffer[2].BufferType = buffer[3].BufferType  = SECBUFFER_EMPTY;

                buffer_desc.ulVersion       = SECBUFFER_VERSION;
                buffer_desc.cBuffers        = 4;
                buffer_desc.pBuffers        = buffer;

                e_status = DecryptMessage(&m_context, &buffer_desc, 0, 0);
                print_error(e_status, "decrypt() ~ DecryptMessage()");
                for (int n=0; n<4; n++)
                    std::cout << "buffer[" << n << "].cbBuffer: " << buffer[n].cbBuffer << "\t"
                    << buffer[n].BufferType << "\n";
                // Locate data and (optional) extra buffers.
                SecBuffer *pDataBuffer  = NULL;
                SecBuffer *pExtraBuffer = NULL;
                for (int i = 1; i < 4; i++) {
                    if (pDataBuffer == NULL && buffer[i].BufferType == SECBUFFER_DATA) {
                        pDataBuffer = &buffer[i];
                        printf("buffer[%d].BufferType = SECBUFFER_DATA\n",i);
                    }
                    if (pExtraBuffer == NULL && buffer[i].BufferType == SECBUFFER_EXTRA) {
                        pExtraBuffer = &buffer[i];
                    }
                }
                if (e_status == SEC_E_OK) {
                    std::string decrypted(reinterpret_cast<const char*>(pDataBuffer->pvBuffer), pDataBuffer->cbBuffer);
                    m_handler->handleDecryptedData(decrypted);
                    if (pExtraBuffer == NULL) m_buffer.clear();
                    else {
                        std::cout << "m_buffer.size() = " << pExtraBuffer->cbBuffer << std::endl;
                        m_buffer.erase(0, m_buffer.size() - pExtraBuffer->cbBuffer);
                        std::cout << "m_buffer.size() = " << m_buffer.size() << std::endl;
                    }
                } else if (e_status == SEC_E_INCOMPLETE_MESSAGE) {
                    break;
                } else {
                    std::cout << "decrypt !!!ERROR!!!\n";
                    system("PAUSE");
                    // throw an error
                    exit(1);
                    break;
                }
            } while (m_buffer.size() != 0);
            LocalFree(e_iobuffer);
        } else {
            handshake_stage(data);
        }
        printf("<< SChannel::decrypt()\n");
        return 0;
    }

    void SChannel::cleanup() {}

    bool SChannel::handshake() {
        printf(">> SChannel::handshake()\n");
        SECURITY_STATUS error;
        ULONG return_flags;
        TimeStamp t;
        SecBuffer obuf[1];
        SecBufferDesc obufs;
        SCHANNEL_CRED tlscred;
        ULONG request = ISC_REQ_ALLOCATE_MEMORY
                        | ISC_REQ_CONFIDENTIALITY
                        | ISC_REQ_EXTENDED_ERROR
                        | ISC_REQ_INTEGRITY
                        | ISC_REQ_REPLAY_DETECT
                        | ISC_REQ_SEQUENCE_DETECT
                        | ISC_REQ_STREAM
                        | ISC_REQ_MANUAL_CRED_VALIDATION;

        /* initialize TLS credential */
        memset (&tlscred,0,sizeof (SCHANNEL_CRED));
        tlscred.dwVersion = SCHANNEL_CRED_VERSION;
        tlscred.grbitEnabledProtocols = SP_PROT_TLS1;
        /* acquire credentials */
        error = AcquireCredentialsHandle(   0,
                                            UNISP_NAME,
                                            SECPKG_CRED_OUTBOUND,
                                            0,
                                            &tlscred,
                                            0,
                                            0,
                                            &m_cred_handle,
                                            &t);
        print_error(error, "handshake() ~ AcquireCredentialsHandle()");
        if (error != SEC_E_OK) {
            return false;
        } else {
            /* initialize buffers */
            obuf[0].cbBuffer = 0;
            obuf[0].pvBuffer = 0;
            obuf[0].BufferType = SECBUFFER_TOKEN;
            /* initialize buffer descriptors */
            obufs.ulVersion = SECBUFFER_VERSION;
            obufs.cBuffers = 1;
            obufs.pBuffers = obuf;
            /* negotiate security */
            SEC_CHAR *hname = const_cast<char*>(m_server.c_str());

            error = InitializeSecurityContext(  &m_cred_handle,
                                                0,
                                                hname,
                                                request,
                                                0,
                                                SECURITY_NETWORK_DREP,
                                                0,
                                                0,
                                                &m_context,
                                                &obufs,
                                                &return_flags,
                                                NULL);
            print_error(error, "handshake() ~ InitializeSecurityContext()");

            if (error == SEC_E_OK) {
                return false;
            } else if (error == SEC_I_CONTINUE_NEEDED) {
                std::cout << "obuf[1].cbBuffer: " << obuf[0].cbBuffer << "\n";
                std::string senddata(static_cast<char*>(obuf[0].pvBuffer), obuf[0].cbBuffer);
                FreeContextBuffer (obuf[0].pvBuffer);
                m_handler->handleEncryptedData(senddata);
                return true;
            } else {}
        }
        return true;
    }

    void SChannel::handshake_stage(const std::string &data) {
        printf(" >> handshake_stage\n");
        m_buffer += data;

        SECURITY_STATUS error;
        ULONG a;
        TimeStamp t;
        SecBuffer ibuf[2],obuf[1];
        SecBufferDesc ibufs,obufs;
        ULONG request = ISC_REQ_ALLOCATE_MEMORY
                        | ISC_REQ_CONFIDENTIALITY
                        | ISC_REQ_EXTENDED_ERROR
                        | ISC_REQ_INTEGRITY
                        | ISC_REQ_REPLAY_DETECT
                        | ISC_REQ_SEQUENCE_DETECT
                        | ISC_REQ_STREAM
                        | ISC_REQ_MANUAL_CRED_VALIDATION;

        SEC_CHAR *hname = const_cast<char*>(m_server.c_str());

        do {
            /* initialize buffers */
            ibuf[0].cbBuffer = m_buffer.size();
            ibuf[0].pvBuffer = static_cast<void*>(const_cast<char*>(m_buffer.c_str()));
            std::cout << "Size: " << m_buffer.size() << "\n";
            ibuf[1].cbBuffer = 0;
            ibuf[1].pvBuffer = 0;
            obuf[0].cbBuffer = 0;
            obuf[0].pvBuffer = 0;

            ibuf[0].BufferType = SECBUFFER_TOKEN;
            ibuf[1].BufferType = SECBUFFER_EMPTY;
            obuf[0].BufferType = SECBUFFER_EMPTY;
            /* initialize buffer descriptors */
            ibufs.ulVersion = obufs.ulVersion = SECBUFFER_VERSION;
            ibufs.cBuffers = 2;
            obufs.cBuffers = 1;
            ibufs.pBuffers = ibuf;
            obufs.pBuffers = obuf;

            std::cout << "obuf[0].cbBuffer: " << obuf[0].cbBuffer << "\t" << obuf[0].BufferType << "\n";
            std::cout << "ibuf[0].cbBuffer: " << ibuf[0].cbBuffer << "\t" << ibuf[0].BufferType << "\n";
            std::cout << "ibuf[1].cbBuffer: " << ibuf[1].cbBuffer << "\t" << ibuf[1].BufferType << "\n";


            /* negotiate security */
            error = InitializeSecurityContext(  &m_cred_handle,
                                                &m_context,
                                                hname,
                                                request,
                                                0,
                                                0,
                                                &ibufs,
                                                0,
                                                0,
                                                &obufs,
                                                &a,
                                                &t);
            print_error(error, "handshake() ~ InitializeSecurityContext()");
            if (error == SEC_E_OK) {
                // EXTRA STUFF??
                if ( ibuf[1].BufferType == SECBUFFER_EXTRA ) {
                    m_buffer.erase(0, m_buffer.size() - ibuf[1].cbBuffer);
                } else {
                    m_buffer = "";
                }
                set_sizes();

                setCertinfos();

                m_secure = true;
                m_handler->handleHandshakeResult(true, m_certInfo);

            } else if (error == SEC_I_CONTINUE_NEEDED) {

                std::cout << "obuf[0].cbBuffer: " << obuf[0].cbBuffer << "\t" << obuf[0].BufferType << "\n";
                std::cout << "ibuf[0].cbBuffer: " << ibuf[0].cbBuffer << "\t" << ibuf[0].BufferType << "\n";
                std::cout << "ibuf[1].cbBuffer: " << ibuf[1].cbBuffer << "\t" << ibuf[1].BufferType << "\n";
                // STUFF TO SEND??
                if (obuf[0].cbBuffer != 0 && obuf[0].pvBuffer != NULL) {
                    std::string senddata(static_cast<char*>(obuf[0].pvBuffer), obuf[0].cbBuffer);
                    FreeContextBuffer(obuf[0].pvBuffer);
                    m_handler->handleEncryptedData(senddata);
                }
                // EXTRA STUFF??
                if ( ibuf[1].BufferType == SECBUFFER_EXTRA ) {
                    m_buffer.erase(0, m_buffer.size() - ibuf[1].cbBuffer);
                } else {
                    m_buffer = "";
                }
                return;
            } else if (error == SEC_I_INCOMPLETE_CREDENTIALS) {
                handshake_stage("");
            } else if (error == SEC_E_INCOMPLETE_MESSAGE) {
                break;
            }
            else if (error == SEC_E_INVALID_TOKEN) break;
            else break;
        } while (true);
    }

void SChannel::setCACerts( const StringList& cacerts ) {}

    void SChannel::setClientCert( const std::string& clientKey, const std::string& clientCerts ) {}

    void SChannel::set_sizes() {
        if (QueryContextAttributes(&m_context, SECPKG_ATTR_STREAM_SIZES, &m_sizes) == SEC_E_OK) {
            std::cout << "set_sizes success\n";
        } else {
            std::cout << "set_sizes no success\n";
        }
    }

    bool SChannel::validateCert() {
        bool valid = false;
        HTTPSPolicyCallbackData policyHTTPS;
        CERT_CHAIN_POLICY_PARA policyParameter;
        CERT_CHAIN_POLICY_STATUS policyStatus;


        PCCERT_CONTEXT remoteCertContext = NULL;
        PCCERT_CHAIN_CONTEXT chainContext = NULL;
        CERT_CHAIN_PARA chainParameter;
        PSTR serverName = const_cast<char*>(m_server.c_str());

        PWSTR   uServerName = NULL;
        DWORD   csizeServerName;

        LPSTR Usages[] = {  szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };
        DWORD cUsages = sizeof(Usages) / sizeof(LPSTR);

        do {
            // Get server's certificate.
            if (QueryContextAttributes(&m_context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (PVOID)&remoteCertContext) != SEC_E_OK) {
                printf("Error querying remote certificate\n");
                // !!! THROW SOME ERROR
                break;
            }

            // unicode conversation
            // calculating unicode server name size
            csizeServerName = MultiByteToWideChar(CP_ACP, 0, serverName, -1, NULL, 0);
            uServerName =reinterpret_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, csizeServerName * sizeof(WCHAR)));
            if (uServerName == NULL) {
                printf("SEC_E_INSUFFICIENT_MEMORY ~ Not enough memory!!!\n");
                break;
            }

            // convert into unicode
            csizeServerName = MultiByteToWideChar(CP_ACP, 0, serverName, -1, uServerName, csizeServerName);
            if (csizeServerName == 0) {
                printf("SEC_E_WRONG_PRINCIPAL\n");
                break;
            }

            // create the chain
            ZeroMemory(&chainParameter, sizeof(chainParameter));
            chainParameter.cbSize = sizeof(chainParameter);
            chainParameter.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
            chainParameter.RequestedUsage.Usage.cUsageIdentifier     = cUsages;
            chainParameter.RequestedUsage.Usage.rgpszUsageIdentifier = Usages;

            if (!CertGetCertificateChain(   NULL, remoteCertContext, NULL, remoteCertContext->hCertStore,
                                            &chainParameter, 0, NULL, &chainContext)) {
                DWORD status = GetLastError();
                printf("Error 0x%x returned by CertGetCertificateChain!!!\n", status);
                break;
            }

            // validate the chain
            ZeroMemory(&policyHTTPS, sizeof(HTTPSPolicyCallbackData));
            policyHTTPS.cbStruct           = sizeof(HTTPSPolicyCallbackData);
            policyHTTPS.dwAuthType         = AUTHTYPE_SERVER;
            policyHTTPS.fdwChecks          = 0;
            policyHTTPS.pwszServerName     = uServerName;

            memset(&policyParameter, 0, sizeof(policyParameter));
            policyParameter.cbSize            = sizeof(policyParameter);
            policyParameter.pvExtraPolicyPara = &policyHTTPS;

            memset(&policyStatus, 0, sizeof(policyStatus));
            policyStatus.cbSize = sizeof(policyStatus);

            if (!CertVerifyCertificateChainPolicy(  CERT_CHAIN_POLICY_SSL, chainContext, &policyParameter,
                                                    &policyStatus)) {
                DWORD status = GetLastError();
                printf("Error 0x%x returned by CertVerifyCertificateChainPolicy!!!\n", status);
                break;
            }

            if (policyStatus.dwError) {
                printf("Trust Error!!!}n");
                break;
            }
            valid = true;
        } while (false);
        // cleanup
        if (chainContext) CertFreeCertificateChain(chainContext);
        return valid;
    }

    void SChannel::setCertinfos() {
        m_certInfo.chain = validateCert();
    }

    void SChannel::print_error(int errorcode, const char *place) {
        printf("Win error at %s.\n", place);
        switch (errorcode) {
        case SEC_E_OK:
            printf("\tValue:\tSEC_E_OK\n");
            printf("\tDesc:\tNot really an error. Everything is fine.\n");
            break;
        case SEC_E_INSUFFICIENT_MEMORY:
            printf("\tValue:\tSEC_E_INSUFFICIENT_MEMORY\n");
            printf("\tDesc:\tThere is not enough memory available to complete the requested action.\n");
            break;
        case SEC_E_INTERNAL_ERROR:
            printf("\tValue:\tSEC_E_INTERNAL_ERROR\n");
            printf("\tDesc:\tAn error occurred that did not map to an SSPI error code.\n");
            break;
        case SEC_E_NO_CREDENTIALS:
            printf("\tValue:\tSEC_E_NO_CREDENTIALS\n");
            printf("\tDesc:\tNo credentials are available in the security package.\n");
            break;
        case SEC_E_NOT_OWNER:
            printf("\tValue:\tSEC_E_NOT_OWNER\n");
            printf("\tDesc:\tThe caller of the function does not have the necessary credentials.\n");
            break;
        case SEC_E_SECPKG_NOT_FOUND:
            printf("\tValue:\tSEC_E_SECPKG_NOT_FOUND\n");
            printf("\tDesc:\tThe requested security package does not exist. \n");
            break;
        case SEC_E_UNKNOWN_CREDENTIALS:
            printf("\tValue:\tSEC_E_UNKNOWN_CREDENTIALS\n");
            printf("\tDesc:\tThe credentials supplied to the package were not recognized.\n");
            break;
        case SEC_E_INCOMPLETE_MESSAGE:
            printf("\tValue:\tSEC_E_INCOMPLETE_MESSAGE\n");
            printf("\tDesc:\tData for the whole message was not read from the wire.\n");
            break;
        case SEC_E_INVALID_HANDLE:
            printf("\tValue:\tSEC_E_INVALID_HANDLE\n");
            printf("\tDesc:\tThe handle passed to the function is invalid.\n");
            break;
        case SEC_E_INVALID_TOKEN:
            printf("\tValue:\tSEC_E_INVALID_TOKEN\n");
            printf("\tDesc:\tThe error is due to a malformed input token, such as a token corrupted in transit...\n");
            break;
        case SEC_E_LOGON_DENIED:
            printf("\tValue:\tSEC_E_LOGON_DENIED\n");
            printf("\tDesc:\tThe logon failed.\n");
            break;
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:
            printf("\tValue:\tSEC_E_NO_AUTHENTICATING_AUTHORITY\n");
            printf("\tDesc:\tNo authority could be contacted for authentication...\n");
            break;
        case SEC_E_TARGET_UNKNOWN:
            printf("\tValue:\tSEC_E_TARGET_UNKNOWN\n");
            printf("\tDesc:\tThe target was not recognized.\n");
            break;
        case SEC_E_UNSUPPORTED_FUNCTION:
            printf("\tValue:\tSEC_E_UNSUPPORTED_FUNCTION\n");
            printf("\tDesc:\tAn invalid context attribute flag (ISC_REQ_DELEGATE or ISC_REQ_PROMPT_FOR_CREDS)...\n");
            break;
        case SEC_E_WRONG_PRINCIPAL:
            printf("\tValue:\tSEC_E_WRONG_PRINCIPAL\n");
            printf("\tDesc:\tThe principal that received the authentication request is not the same as the...\n");
            break;

        case SEC_I_COMPLETE_AND_CONTINUE:
            printf("\tValue:\tSEC_I_COMPLETE_AND_CONTINUE\n");
            printf("\tDesc:\tThe client must call CompleteAuthToken and then pass the output...\n");
            break;
        case SEC_I_COMPLETE_NEEDED:
            printf("\tValue:\tSEC_I_COMPLETE_NEEDED\n");
            printf("\tDesc:\tThe client must finish building the message and then call the CompleteAuthToken function.\n");
            break;
        case SEC_I_CONTINUE_NEEDED:
            printf("\tValue:\tSEC_I_CONTINUE_NEEDED\n");
            printf("\tDesc:\tThe client must send the output token to the server and wait for a return token...\n");
            break;
        case SEC_I_INCOMPLETE_CREDENTIALS:
            printf("\tValue:\tSEC_I_INCOMPLETE_CREDENTIALS\n");
            printf("\tDesc:\tThe server has requested client authentication, and the supplied credentials either...\n");
            break;
        default:
            printf("\tValue:\t%d\n", errorcode);
            printf("\tDesc:\tUnknown error code.\n");
        }
    }
}

async function updateStatus() {
    const response = await fetch('/status');
    const data = await response.json();
    const statusText = document.getElementById("status-text");
    const statusIcon = document.querySelector('.status-conexao i');
    
    const sendButton = document.getElementById("send-button");
    const startButton = document.getElementById("start-button");
    const stopButton = document.getElementById("stop-button");

    const status = data.status.toLowerCase();

    if (status === "conectado") {
        statusText.textContent = "Conectado";
        statusIcon.className = "fas fa-circle conectado";

        if (sendButton) sendButton.disabled = false;
        if (startButton) startButton.disabled = false;
        if (stopButton) stopButton.disabled = true;

    } else if (status === "desconectado") {
        statusText.textContent = "Desconectado";
        statusIcon.className = "fas fa-circle desconectado";

        if (sendButton) sendButton.disabled = true;
        if (startButton) startButton.disabled = true;
        if (stopButton) stopButton.disabled = true;

    } else {
        statusText.textContent = data.status;
        statusIcon.className = "fas fa-spinner tentando";
    
        const isContinuousExperiment = data.status.startsWith("Executando experimento");
    
        if (sendButton) sendButton.disabled = true;
        if (startButton) startButton.disabled = true;
        if (stopButton) stopButton.disabled = !isContinuousExperiment;
    }
    await updateTable();
    await updateLogTable();
}

async function updateLogTable() {
    try {
        const response = await fetch('/commands');
        const commands = await response.json();

        const tbody = document.getElementById('log-tabela-body');
        if (!tbody) {
            console.warn("Elemento tbody da log-tabela não encontrado.");
            return;
        }

        tbody.innerHTML = ''; // Limpa a tabela

        commands.forEach(cmd => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>${cmd.id}</td>
                <td>${cmd.send_date}</td>
                <td style="font-weight: bold; color: ${cmd.status === 'SUCCESS' ? 'green' : 'red'};">
                    ${cmd.status}
                </td>
            `;
            tbody.appendChild(row);
        });

    } catch (error) {
        console.error("Erro ao atualizar log-tabela:", error);
    }
}


async function updateTable() {
    const response = await fetch('/results');
    const results = await response.json();
    const tbody = document.querySelector('#dynamic-tbody');
    if (!tbody) {
        console.warn("Elemento tbody não encontrado. Aguardando renderização...");
        return;
    }
    tbody.innerHTML = ''; 

    results.forEach(result => {

        let percentError = result.response.data?.percent_error;
        if (!isFinite(percentError)) {
            percentError = 100; // ou qualquer valor alternativo, como "100% ou "---"
        }
        
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${result.receive_date}</td>
            <td style="font-weight: bold; color: ${result.status === 'SUCCESS' ? 'green' : 'red'};">
                ${result.status}
            </td>
            <td>${result.response.data?.id || 'N/D'}</td>
            <td class="data-quantity">${result.response.data?.data_length || 0}</td>
            <td class="corrected-quantity">${result.response.data?.corrected_data || 0}</td>
            <td class="error-quantity">${result.response.data?.number_errors || 0}</td>
            <td>
                <button onclick="baixarCsv('${result.response.data?.id || ''}', '${result.receive_date}')" class="save-button" ${!result.response.data ? 'disabled' : ''}>
                    <i class="fas fa-download"></i>
                </button>
            </td>
        `;
        tbody.appendChild(row);
    }); 
}

async function baixarCsv(experimentId, formattedDate) {
    const response = await fetch(`/baixar_csv/${experimentId}`, {
        method: 'GET',
        headers: {
            'Content-Type': 'application/json',
        },
    });

    if (response.ok) {
        const blob = await response.blob();
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        const safeDate = formattedDate.replace(/[:]/g, '-');
        a.href = url;
        a.download = `experimento_${safeDate}.csv`; // Nome do arquivo
        document.body.appendChild(a);
        a.click();
        a.remove();
        window.URL.revokeObjectURL(url);

        alert('Download iniciado!');
    } else {
        alert("Erro ao tentar baixar o arquivo.");
    }
}

document.addEventListener('DOMContentLoaded', function () {
    const cmdType = document.getElementById('cmd_type');
    if (!cmdType) return; // <- Protege caso o elemento não exista

    const fieldGroups = {
        data_length: true,
        repeat: true,
        write_type: true,
        read: true,
        delay: true,
        offset: true,
        estimated_time: true,
    };

    const hideFieldsForCmdType = {
        '' : ['repeat', 'estimated_time'],
        '1': ['repeat', 'estimated_time'],
        '3': ['read']
    };

    function toggleFields(cmdValue) {
        const fieldsToHide = hideFieldsForCmdType[cmdValue] || [];

        for (const field in fieldGroups) {
            const div = document.getElementById(`${field}_div`);
            if (!div) continue;
            const input = div.querySelector('input, select');
            if (!input) continue;

            const shouldHide = fieldsToHide.includes(field);
            div.style.display = shouldHide ? 'none' : 'block';
            input.disabled = shouldHide;
        }

        const sendBtn = document.getElementById('send-button');
        const startBtn = document.getElementById('start-button');
        const stopBtn = document.getElementById('stop-button');

        if (cmdValue === '3') {
            if (sendBtn) sendBtn.style.display = 'none';
            if (startBtn) startBtn.style.display = 'inline-block';
            if (stopBtn) stopBtn.style.display = 'inline-block';
        } else {
            if (sendBtn) sendBtn.style.display = 'inline-block';
            if (startBtn) startBtn.style.display = 'none';
            if (stopBtn) stopBtn.style.display = 'none';
        }
    }

    cmdType.addEventListener('change', function () {
        toggleFields(cmdType.value);
    });

    // Trigger inicial
    toggleFields(cmdType.value);
});


function updateEstimatedTime() {
    const delayInput = document.getElementById('delay');
    const repeatInput = document.getElementById('repeat');
    const estimatedTime = document.getElementById('estimated_time');

    const delay = parseFloat(delayInput.value) || 0;
    const repeat = parseInt(repeatInput.value) || 1;

    const total = delay * repeat;
    estimatedTime.value = total.toFixed(2); // exibe com 2 casas decimais
}

document.addEventListener("DOMContentLoaded", function () {
    const cmdType = document.getElementById('cmd_type');
    if (!cmdType) return; 

    const escrita = document.getElementById('write_type');
    const delay = document.getElementById('delay');
    const offset = document.getElementById('offset');
    const ecc = document.getElementById('ecc');
    const repeat = document.getElementById('repeat');

    function atualizarObrigatoriedadeCampos() {
        [escrita, delay, offset, ecc, repeat].forEach(field => field.removeAttribute('required'));

        const tipo = cmdType.value;

        // Define regras de obrigatoriedade baseadas no tipo de comando
        if (tipo === 'WRITE_CMD') {
            delay.setAttribute('required', 'required');
            escrita.setAttribute('required', 'required');
            ecc.setAttribute('required', 'required');
        } else if (tipo === 'READ_CMD') {
            offset.setAttribute('required', 'required');
        } else if (tipo === 'CONTINUOUS_CMD') {
            delay.setAttribute('required', 'required');
            repeat.setAttribute('required', 'required');
        }
    }

    // Atualiza os campos ao trocar o tipo
    cmdType.addEventListener('change', atualizarObrigatoriedadeCampos);

    // Executa uma vez ao carregar a página (se já tiver um valor selecionado)
    atualizarObrigatoriedadeCampos();
});

window.addEventListener("DOMContentLoaded", () => {
    const sendButton = document.getElementById("send-button");
    if (sendButton) {
        sendButton.addEventListener("click", async (event) => {
            event.preventDefault();
            const form = document.getElementById('experiment-form');
            const formData = new FormData(form);
            try {
                const response = await fetch('/new_command', {
                    method: 'POST',
                    body: formData
                });

                if (response.redirected) {
                    window.location.href = response.url;
                } else if (response.ok) {
                    updateStatus();
                    showNotification("Comando enviado com sucesso!", "success");
                } else {
                    showNotification("Erro ao enviar comando.", "error");
                }
            } catch (error) {
                console.error("Erro ao enviar:", error);
                showNotification("Erro de conexão.", "error");
            }
            await updateLogTable();
        });
    }
});

const startButton = document.getElementById('start-button');
if (startButton) {
    startButton.addEventListener('click', async () => {
        const form = document.getElementById('experiment-form'); 
        if (!form) return;
        const formData = new FormData(form);
        
        const keysToDelete = [];

        for (const [key, value] of formData.entries()) {
            if (value.trim() === "") {
                keysToDelete.push(key);
            }
        }

        for (const key of keysToDelete) {
            formData.delete(key);
        }

        console.log("FormData conteúdo:");
        for (const [key, value] of formData.entries()) {
            console.log(`${key}: ${value}`);
        }
        
        try {
           
            const response = await fetch('/start_continuous', {
                method: 'POST',
                body: formData
            });

            if (response.ok) {
                updateStatus();
                showNotification("Experimento iniciado com sucesso!", "success");
            } else {
                const errorText = await response.text();
                console.error("Erro do servidor:", errorText);
                showNotification("Erro ao iniciar o experimento.", "error");
            }
        } catch (error) {
            console.error("Erro ao iniciar:", error);
            showNotification("Erro de conexão.", "error");
        }
        await updateLogTable();
    });
}

// Botão "Parar" o experimento
const stopButton = document.getElementById('stop-button');
if (stopButton) {
    stopButton.addEventListener('click', async () => {
        try {
            const response = await fetch('/stop_experiment', {
                method: 'POST'
            });

            if (response.ok) {
                updateStatus();
                showNotification("Parando realização do experimento!", "success");
            } else {
                showNotification("Erro ao parar o experimento.", "error");
            }
        } catch (error) {
            console.error("Erro ao parar:", error);
            showNotification("Erro de conexão.", "error");
        }
        await updateLogTable();
    });
}

function showNotification(message, type = 'success') {
    const container = document.getElementById('notification-container');
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.textContent = message;
    container.appendChild(notification);

    setTimeout(() => {
        notification.remove();
    }, 4000);
}

const delayInput = document.getElementById('delay');
const repeatInput = document.getElementById('repeat');

if (delayInput && repeatInput) {
    delayInput.addEventListener('input', updateEstimatedTime);
    repeatInput.addEventListener('input', updateEstimatedTime);
    updateEstimatedTime(); 
}

// Atualiza o formulário quando a página carregar pela primeira vez e enicializa a função de atualização quando a página carregar
window.onload = function () {
    updateStatus();
    setInterval(updateStatus, 3000);
};

